
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp> 

// ESP32でLovyanGFXを独自設定で利用する場合の設定例

/// 独自の設定を行うクラスを、LGFX_Deviceから派生して作成します。
class MYLGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341     _panel_instance;
  lgfx::Bus_SPI        _bus_instance;   
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046          _touch_instance;

public:

  // コンストラクタを作成し、ここで各種設定を行います。
  // クラス名を変更した場合はコンストラクタも同じ名前を指定してください。
  MYLGFX(void)
  {
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。

// SPIバスの設定
      cfg.spi_host = VSPI_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = false;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = 25;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 32;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = 39;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 33;            // SPIのD/Cピン番号を設定  (-1 = disable)
     // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。
//*/
/*
// I2Cバスの設定
      cfg.i2c_port    = 0;          // 使用するI2Cポートを選択 (0 or 1)
      cfg.freq_write  = 400000;     // 送信時のクロック
      cfg.freq_read   = 400000;     // 受信時のクロック
      cfg.pin_sda     = 21;         // SDAを接続しているピン番号
      cfg.pin_scl     = 22;         // SCLを接続しているピン番号
      cfg.i2c_addr    = 0x3C;       // I2Cデバイスのアドレス
//*/
/*
// 8ビットパラレルバスの設定
      cfg.i2s_port = I2S_NUM_0;     // 使用するI2Sポートを選択 (I2S_NUM_0 or I2S_NUM_1) (ESP32のI2S LCDモードを使用します)
      cfg.freq_write = 20000000;    // 送信クロック (最大20MHz, 80MHzを整数で割った値に丸められます)
      cfg.pin_wr =  4;              // WR を接続しているピン番号
      cfg.pin_rd =  2;              // RD を接続しているピン番号
      cfg.pin_rs = 15;              // RS(D/C)を接続しているピン番号
      cfg.pin_d0 = 12;              // D0を接続しているピン番号
      cfg.pin_d1 = 13;              // D1を接続しているピン番号
      cfg.pin_d2 = 26;              // D2を接続しているピン番号
      cfg.pin_d3 = 25;              // D3を接続しているピン番号
      cfg.pin_d4 = 17;              // D4を接続しているピン番号
      cfg.pin_d5 = 16;              // D5を接続しているピン番号
      cfg.pin_d6 = 27;              // D6を接続しているピン番号
      cfg.pin_d7 = 14;              // D7を接続しているピン番号
//*/

      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs           =    14;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    -1;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

      cfg.panel_width      =   240;  // 実際に表示可能な幅
      cfg.panel_height     =   320;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

// 以下はST7735やILI9163のようにピクセル数が可変のドライバで表示がずれる場合にのみ設定してください。
//    cfg.memory_width     =   240;  // ドライバICがサポートしている最大の幅
//    cfg.memory_height    =   320;  // ドライバICがサポートしている最大の高さ

      _panel_instance.config(cfg);
    }

//*
    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

      cfg.pin_bl = 32;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
//*/

//*
    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();

      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = 239;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = 319;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int    = 38;   // INTが接続されているピン番号
      cfg.bus_shared = true; // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 0;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定

// SPI接続の場合
      cfg.spi_host = VSPI_HOST;// 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;     // SPIクロックを設定
      cfg.pin_sclk = 18;     // SCLKが接続されているピン番号
      cfg.pin_mosi = 23;     // MOSIが接続されているピン番号
      cfg.pin_miso = 19;     // MISOが接続されているピン番号
      cfg.pin_cs   =  5;     //   CSが接続されているピン番号

// I2C接続の場合
      cfg.i2c_port = 1;      // 使用するI2Cを選択 (0 or 1)
      cfg.i2c_addr = 0x38;   // I2Cデバイスアドレス番号
      cfg.pin_sda  = 23;     // SDAが接続されているピン番号
      cfg.pin_scl  = 32;     // SCLが接続されているピン番号
      cfg.freq = 400000;     // I2Cクロックを設定

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
//*/

    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

// 準備したクラスのインスタンスを作成します。
LGFX lcd;

void setup(void)
{
  // SPIバスとパネルの初期化を実行すると使用可能になります。
  lcd.init();

  lcd.setTextSize((std::max(lcd.width(), lcd.height()) + 255) >> 8);

  // タッチが使用可能な場合のキャリブレーションを行います。（省略可）
  if (lcd.touch())
  {
    if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

    // 画面に案内文章を描画します。
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.drawString("touch the arrow marker.", lcd.width()>>1, lcd.height() >> 1);
    lcd.setTextDatum(textdatum_t::top_left);

    // タッチを使用する場合、キャリブレーションを行います。画面の四隅に表示される矢印の先端を順にタッチしてください。
    std::uint16_t fg = TFT_WHITE;
    std::uint16_t bg = TFT_BLACK;
    if (lcd.isEPD()) std::swap(fg, bg);
    lcd.calibrateTouch(nullptr, fg, bg, std::max(lcd.width(), lcd.height()) >> 3);
  }

  lcd.fillScreen(TFT_BLACK);
}

uint32_t count = ~0;
void loop(void)
{
  lcd.startWrite();
  lcd.setRotation(++count & 7);
  lcd.setColorDepth((count & 8) ? 16 : 24);

  lcd.setTextColor(TFT_WHITE);
  lcd.drawNumber(lcd.getRotation(), 16, 0);

  lcd.setTextColor(0xFF0000U);
  lcd.drawString("R", 30, 16);
  lcd.setTextColor(0x00FF00U);
  lcd.drawString("G", 40, 16);
  lcd.setTextColor(0x0000FFU);
  lcd.drawString("B", 50, 16);

  lcd.drawRect(30,30,lcd.width()-60,lcd.height()-60,count*7);
  lcd.drawFastHLine(0, 0, 10);

  lcd.endWrite();

  int32_t x, y;
  if (lcd.getTouch(&x, &y)) {
    lcd.fillRect(x-2, y-2, 5, 5, count*7);
  }
}
