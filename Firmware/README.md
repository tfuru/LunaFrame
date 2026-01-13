# PopLinkBadge

PopLinkBadgeは、ESP32-S3マイクロコントローラと円形ディスプレイGC9A01を搭載したバッジ型デバイスです。  
Wi-Fi APモードで動作し、表示画像をスマートフォンなどから簡単に更新できます。  

このディレクトリは、PopLinkBadgeのファームウェアソースコードを格納しています。

## 特徴
- ESP32-S3搭載
- GC9A01 円形LCDディスプレイ (240x240)
- Wi-Fi APモードによる簡単な画像更新
- 表示画像フォーマット: PNG (240x240)
- 表示画像フォーマット: PNG (240x240)

## ハードウェア構成

### メインIC
- Microcontroller: ESP32-S3
- Display: GC9A01

### 接続ピン
| GC9A01 Pin | ESP32-S3 GPIO |
| :--- | :--- |
| VCC | 3.3V |
| GND | GND |
| SCL (SCLK) | 7 |
| SDA (MOSI) | 9 |
| DC | 8 |
| CS | 6 |
| RES (RST) | 5 |

## 開発環境
このプロジェクトは[PlatformIO](https://platformio.org/)を使用して開発されています。

- **Board:** `seeed_xiao_esp32s3`, `seeed_xiao_esp32c3`, `seeed_xiao_esp32c3`
- **Platform:** `espressif32`
- **Framework:** `arduino`

### ライブラリ
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX): グラフィックライブラリ

依存ライブラリは `platformio.ini` に記載されており、ビルド時に自動的にインストールされます。

## 使用方法

### 1. ファームウェアの書き込み
1. [VSCodeにPlatformIO IDE拡張機能](https://platformio.org/install/ide?install=vscode)をインストールします。
2. このプロジェクトをPlatformIOで開きます。
3. PopLinkBadgeデバイスをPCに接続します。
4. PlatformIOの "Upload" タスクを実行して、ファームウェアを書き込みます。

### 2. 画像の更新
1. PopLinkBadgeデバイスの電源を入れ、Wi-Fi設定画面から `PopLinkBadge` ネットワークに接続します。パスワードは不要です。
2. 表示したい **240x240ピクセルのPNG形式** の画像を用意し、ファイル名を `image.png` に変更します。
3. スマートフォンやPCのブラウザで `http://192.168.4.1` にアクセスします。
4. Slot 1 〜 Slot 5 のいずれかに画像をアップロードします。
5. 画像がアップロードされると、デバイスの画面に即座に反映されます。複数の画像をアップロードすると、自動的にスライドショー再生されます。
6. 画像設定後は、一定時間が経過するとWi-Fiがオフになります。

### Command Line (Makefile)
Makefileを使用してコマンドラインからビルドや書き込みを行うこともできます。
環境変数 `ENV` で対象ボードを指定できます（省略時は `platformio.ini` のデフォルトまたは全環境）。

```bash
# プロジェクトのビルド (デフォルト/全環境)
make build

# 特定の環境を指定してビルド (例: Seeed XIAO ESP32C3)
make ENV=seeed_xiao_esp32c3 build

# ファームウェアの書き込み
make upload
make ENV=seeed_xiao_esp32c3 upload

# ファイルシステムの書き込み (dataフォルダ)
make uploadfs
make ENV=seeed_xiao_esp32c3 uploadfs

# シリアルモニターの起動
make monitor
make ENV=seeed_xiao_esp32c3 monitor

# ビルドアーティファクトの削除
make clean
```