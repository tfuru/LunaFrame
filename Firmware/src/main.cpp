#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

// ----------------------------------------------------------------------------
// LovyanGFXのカスタム設定
// ----------------------------------------------------------------------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      // XIAO ESP32C3 Pin Mapping
      cfg.pin_sclk = D7; // SCL
      cfg.pin_mosi = D9; // SDA
      cfg.pin_miso = -1; // MISO
      cfg.pin_dc = D8;   // D/C

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs = D6;  // CS
      cfg.pin_rst = D5; // RST

      cfg.pin_busy = -1; // BUSY
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.readable = true;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
      _panel_instance.setColorDepth(lgfx::v1::color_depth_t::rgb565_2Byte);
    }
    setPanel(&_panel_instance);
  }
};

// ----------------------------------------------------------------------------
// グローバル変数
// ----------------------------------------------------------------------------
static LGFX lcd;
AsyncWebServer server(80);
DNSServer dnsServer;
const char *ssid = "PopLinkBadge";
const char *password = NULL; // パスワードなし

const int MAX_IMAGES = 5;
unsigned long slideInterval = 3000; // スライド切り替え間隔 (3秒)
unsigned long lastSlideTime = 0;
int currentSlideIndex = -1;
unsigned long wifiTimeout = 300000; // Wi-Fiタイムアウト（ミリ秒）：5分
unsigned long lastActivity = 0;
const unsigned long STARTUP_DELAY = 60000; // 起動後待機時間 (1分)
bool forceSlideshow = false;

// モバイルバッテリー対策用
unsigned long lastDummyLoadTime = 0;
const unsigned long DUMMY_LOAD_INTERVAL = 10000; // 10秒ごと
const unsigned long DUMMY_LOAD_DURATION = 200;   // 200ms負荷をかける

// ----------------------------------------------------------------------------
// 関数プロトタイプ
// ----------------------------------------------------------------------------
bool drawImage(String path);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index,
                  uint8_t *data, size_t len, bool final);
void setupWifi();
void setupWebServer();
void updateSlide();
void loadConfig();
void saveConfig();

// ----------------------------------------------------------------------------
// セットアップ
// ----------------------------------------------------------------------------
void setup() {
  // パフォーマンス設定 (モバイルバッテリーの自動OFF対策)
  setCpuFrequencyMhz(240); // CPU周波数を240MHzに設定

  // LCD初期化
  lcd.begin();
  lcd.setRotation(2);
  lcd.setBrightness(128);
  lcd.fillScreen(TFT_BLACK);

  // Debug: 起動確認用テキスト
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.println("Booting...");

  // ファイルシステム初期化
  if (!LittleFS.begin(true, "/littlefs", 10, "storage")) {
    // 中央位置に表示
    lcd.setCursor(20, 100);
    lcd.println("LittleFS Mount Failed");
    while (true) {
      delay(1000);
    } // 失敗時は停止
  }

  // 設定読み込み
  loadConfig();

  // 初期画像表示 (QR.png)
  if (!drawImage("/QR.png")) {
    lcd.setTextSize(2);
    lcd.setCursor(20, 100);
    lcd.println("Upload image");
  }

  // 常にWi-FiとWebサーバーを設定
  setupWifi();
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Wi-Fi出力を最大化
  setupWebServer();
  lastActivity = millis(); // 最終アクティビティを記録
}

// ----------------------------------------------------------------------------
// メインループ
// ----------------------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest();

  // スライドショー更新
  // 起動後 STARTUP_DELAY 経過するまでは更新しない (QRコードを表示し続ける)
  // ただし forceSlideshow が true なら即座に開始
  if ((millis() > STARTUP_DELAY || forceSlideshow) &&
      (millis() - lastSlideTime > slideInterval)) {
    updateSlide();
  }

  // Wi-Fiタイムアウト処理
  if (WiFi.getMode() == WIFI_AP && (millis() - lastActivity > wifiTimeout)) {
    lcd.println("Wi-Fi timeout.");
    WiFi.mode(WIFI_OFF);
  }
  // モバイルバッテリー対策: 定期的なダミー負荷
  if (millis() - lastDummyLoadTime > DUMMY_LOAD_INTERVAL) {
    unsigned long start = millis();
    while (millis() - start < DUMMY_LOAD_DURATION) {
      // ビジーループで電力を消費させる
      __asm__ __volatile__("nop");
    }
    lastDummyLoadTime = millis();
  }

  delay(10);
}

// ----------------------------------------------------------------------------
// 画像表示
// ----------------------------------------------------------------------------
bool drawImage(String path) {
  if (LittleFS.exists(path)) {
    fs::File f = LittleFS.open(path);
    if (f) {
      lgfx::StreamWrapper wrapper;
      wrapper.set(&f);
      lcd.drawPng(&wrapper);
      f.close();
      return true; // 画像を表示した
    }
  }
  return false;
}

// ----------------------------------------------------------------------------
// スライドショー更新
// ----------------------------------------------------------------------------
void updateSlide() {
  bool found = false;
  int nextIndex = currentSlideIndex;

  // 次の有効な画像を探す (最大MAX_IMAGES回試行)
  for (int i = 0; i < MAX_IMAGES; i++) {
    nextIndex = (nextIndex + 1) % MAX_IMAGES;
    String path = "/image" + String(nextIndex) + ".png";
    if (LittleFS.exists(path)) {
      if (currentSlideIndex != nextIndex) {
        lcd.fillScreen(TFT_BLACK);
        drawImage(path);
        currentSlideIndex = nextIndex;
      }
      lastSlideTime = millis();
      found = true;
      break;
    }
  }

  if (!found && currentSlideIndex != -1) {
    // 画像が1つもない場合（削除された場合など）
    currentSlideIndex = -1;
    lcd.fillScreen(TFT_BLACK);
    if (!drawImage("/QR.png")) {
      lcd.setTextSize(2);
      lcd.setCursor(20, 100);
      lcd.println("Upload image");
    }
  } else if (!found && currentSlideIndex == -1) {
    // 既にデフォルト画面(QR.png)で、画像がない場合は何もしない
  }
}

// ----------------------------------------------------------------------------
// Wi-Fi設定
// ----------------------------------------------------------------------------
void setupWifi() {
  WiFi.setSleep(false); // Wi-Fi省電力機能を無効化
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  dnsServer.start(53, "*", IP);
}

// ----------------------------------------------------------------------------
// Webサーバー設定
// ----------------------------------------------------------------------------
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
    lastActivity = millis(); // アクティビティを記録
  });

  server.on(
      "/upload", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Upload Complete.");

        // アップロードされた画像を即座に表示するためにインデックスを調整
        if (request->hasParam("id")) {
          int id = request->getParam("id")->value().toInt();
          if (id >= 0 && id < MAX_IMAGES) {
            currentSlideIndex = id; // 表示対象を強制的に変更
            String path = "/image" + String(id) + ".png";
            lcd.fillScreen(TFT_BLACK);
            drawImage(path);
            lastSlideTime = millis(); // タイマーリセット
          }
        }
      },
      handleUpload);

  server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("id")) {
      int id = request->getParam("id")->value().toInt();
      if (id >= 0 && id < MAX_IMAGES) {
        String path = "/image" + String(id) + ".png";
        if (LittleFS.exists(path)) {
          LittleFS.remove(path);
          // 削除された画像が表示中だった場合、即座に更新
          // (updateSlideで次の画像へ、またはQRへ)
          if (currentSlideIndex == id) {
            updateSlide();
          }
          request->send(200, "text/plain", "Deleted");
        } else {
          request->send(404, "text/plain", "File not found");
        }
      } else {
        request->send(400, "text/plain", "Invalid ID");
      }
    } else {
      request->send(400, "text/plain", "Missing ID");
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
    lastActivity = millis();
  });

  // 静的ファイルの配信 (QR.pngなど)
  server.serveStatic("/", LittleFS, "/");

  server.on("/get-interval", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(slideInterval));
  });

  server.on("/set-interval", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
      int val = request->getParam("value", true)->value().toInt();
      if (val >= 1000 && val <= 60000) { // 1秒〜60秒
        slideInterval = val;
        saveConfig();
        request->send(200, "text/plain", "OK");
      } else {
        request->send(400, "text/plain", "Invalid value");
      }
    } else {
      request->send(400, "text/plain", "Missing value");
    }
  });

  server.on("/start-slideshow", HTTP_POST, [](AsyncWebServerRequest *request) {
    forceSlideshow = true;
    updateSlide(); // 即時切り替えトライ
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}

// ----------------------------------------------------------------------------
// ファイルアップロード処理
// ----------------------------------------------------------------------------
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index,
                  uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    String path = "/image0.png";
    if (request->hasParam("id")) {
      String id = request->getParam("id")->value();
      path = "/image" + id + ".png";
    }
    request->_tempFile = LittleFS.open(path, "w");
  }

  if (len) {
    request->_tempFile.write(data, len);
  }

  if (final) {
    request->_tempFile.close();
    lastActivity = millis(); // アクティビティを記録
  }
}

// ----------------------------------------------------------------------------
// 設定読み込み
// ----------------------------------------------------------------------------
void loadConfig() {
  if (LittleFS.exists("/config.txt")) {
    fs::File f = LittleFS.open("/config.txt", "r");
    if (f) {
      String line = f.readStringUntil('\n');
      line.trim(); // 改行コード除去
      // 簡単なバリデーション (整数変換して0ならデフォルト)
      int val = line.toInt();
      if (val >= 1000) {
        slideInterval = val;
      }
      f.close();
    }
  }
}

// ----------------------------------------------------------------------------
// 設定保存
// ----------------------------------------------------------------------------
void saveConfig() {
  fs::File f = LittleFS.open("/config.txt", "w");
  if (f) {
    f.println(slideInterval);
    f.close();
  }
}
