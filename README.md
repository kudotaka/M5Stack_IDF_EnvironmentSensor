# M5StackとCO2・温度・湿度・気圧な環境センサーをいろいろと試してみる。

### 部品
- [M5Stamp S3](https://www.switch-science.com/products/8777)
- [ATOMS3 Lite](https://www.switch-science.com/products/8778)
- [ATOM PortABC拡張ベース](https://www.switch-science.com/products/9198)
- [M5Stack用SCD40搭載CO2ユニット（温湿度センサ付き）](https://www.switch-science.com/products/8496)
- [M5Stack用温湿度気圧センサユニット Ver.3（ENV Ⅲ）](https://www.switch-science.com/products/7254)
- [M5Stack用気圧センサユニット（QMP6988）](https://www.switch-science.com/products/8663)
- [M5Stack用環境センサユニット ver.2（ENV II）](https://www.switch-science.com/products/6344)
- [Grove - SCD30搭載 CO2・温湿度センサ（Arduino用）](https://www.switch-science.com/products/7000)
- [ＢＭＥ６８０使用　温湿度・気圧・ガスセンサモジュールキット](https://akizukidenshi.com/catalog/g/gK-14469/)
- [ＣＯ２センサーモジュール　ＭＨ－Ｚ１９Ｃ](https://akizukidenshi.com/catalog/g/gM-16142/)
- [ＡＤＴ７４１０使用　高精度・高分解能　Ｉ２Ｃ・１６Ｂｉｔ　温度センサモジュール](https://akizukidenshi.com/catalog/g/gM-06675/)

### 開発環境
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO](https://platformio.org/)
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32s3/index.html)

### 動作
センサーから値を取得できた。
```
MY-MAIN: SENSOR temperature:27.608912 humidity:48.621346 pressure:1009.629150 CO2:517
MY-MAIN: SENSOR temperature:27.592890 humidity:48.648815 pressure:1009.591248 CO2:514
```

### その他
以前のESP-IDFのバージョンで作っていたけど、ESP-IDF v5.1.1で動作するようにアップデートをした。
