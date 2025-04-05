# ESP32-S3 + ST7789 4Dテッセラクトデモ

ESP32-S3マイコンボードとST7789 TFTディスプレイを使用した、Amiga/Demosceneスタイルの4次元テッセラクト（超立方体）デモプログラムです。

このプロジェクトは、ESP32-S3とST7789ディスプレイを使用して、以下の要素を組み合わせた視覚的なデモを実現しています：

- **4次元テッセラクト**: 複数軸で回転する4次元超立方体
- **スターフィールド**: Z軸に沿って動く3D星空エフェクト
- **コッパーバー**: アミーガスタイルの波打つグラデーションバー
- **スクロールテキスト**: 波打ちながら動くスクロールテキスト
- **フラッシュエフェクト**: ランダムな画面フラッシュ効果

## ハードウェア要件

- ESP32-S3開発ボード
- ST7789 240x240 SPI TFTディスプレイ
- 接続ケーブル

## ピン接続

ST7789ディスプレイとESP32-S3の接続は以下の通りです：

| ST7789 | ESP32-S3 |
|--------|----------|
| SCL/SCK | GPIO 12 |
| SDA/MOSI | GPIO 11 |
| DC/RS | GPIO 5 |
| RST | GPIO 4 |
| BLK | GPIO 6 |
| VCC | 3.3V |
| GND | GND |

## ビルド方法

このプロジェクトはESP-IDF環境を使用してビルドします。

1. [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)をインストール
2. リポジトリをクローン：
   ```bash
   git clone https://github.com/mtts2/esp32s3_st7789.git
   cd esp32s3_st7789
   ```
3. ビルドとフラッシュ：
   ```bash
   idf.py build
   idf.py -p (COMポート) flash
   ```

## 依存ライブラリ

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) - グラフィックライブラリ

LovyanGFXは優れたグラフィックライブラリでESP32-S3でのグラフィック処理を大幅に簡素化します。別途コンポーネントとしてプロジェクトに追加する必要があります。

## プロジェクト構造

- **main/esp32s3_st7789.cpp**: メイン実装コード
- **main/st7789.hpp**: ST7789ディスプレイ設定用のヘッダーファイル
- **CMakeLists.txt**: CMakeプロジェクト設定ファイル

## カスタマイズ

- ディスプレイの向き: `lcd.setRotation(0)` を変更（0-3）
- ピン配置: `st7789.hpp` 内の設定を変更

## ライセンス

MIT

## 謝辞

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) - 素晴らしいグラフィックライブラリを提供してくださった[lovyan03](https://github.com/lovyan03)氏に感謝します
- Amiga Demosceneのアーティストたち - このプロジェクトのインスピレーション源

---

作成者: [@mtts2](https://github.com/mtts2)
