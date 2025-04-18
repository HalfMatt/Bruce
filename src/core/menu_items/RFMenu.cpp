#include "RFMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/rf/record.h"
#include "modules/rf/rf.h"

void RFMenu::optionsMenu() {
    options = {
        {"Scan/copy",       rf_scan_copy           },
        {"Record RAW",      rf_raw_record          }, //  Pablo-Ortiz-Lopez
        {"Custom SubGhz",   otherRFcodes           },
        {"Spectrum",        rf_spectrum            }, //@IncursioHack
        {"SquareWave Spec", rf_SquareWave          }, //@Pirata
        {"Jammer Itmt",     rf_jammerIntermittent  }, //@IncursioHack
        {"Jammer Full",     rf_jammerFull          }, //@IncursioHack
        {"Config",          [=]() { configMenu(); }},
    };
    addOptionToMainMenu();

    delay(200);
    String txt = "Radio Frequency";
    if (bruceConfig.rfModule == CC1101_SPI_MODULE) txt += " (CC1101)"; // Indicates if CC1101 is connected
    else txt += " Tx: " + String(bruceConfig.rfTx) + " Rx: " + String(bruceConfig.rfRx);

    loopOptions(options, true, txt.c_str());
}

void RFMenu::configMenu() {
    options = {
        {"RF TX Pin", lambdaHelper(gsetRfTxPin, true)},
        {"RF RX Pin", lambdaHelper(gsetRfRxPin, true)},
        {"RF Module", setRFModuleMenu},
        {"RF Frequency", setRFFreqMenu},
        {"Back", [=]() { optionsMenu(); }},
    };

    loopOptions(options, true, "RF Config");
}
void RFMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.rf), 0, imgCenterY, true
    );
}
void RFMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 7;
    int deltaRadius = scale * 10;
    int triangleSize = scale * 30;

    if (triangleSize % 2 != 0) triangleSize++;

    // Body
    tft.fillCircle(iconCenterX, iconCenterY - radius, radius, bruceConfig.priColor);
    tft.fillTriangle(
        iconCenterX,
        iconCenterY,
        iconCenterX - triangleSize / 2,
        iconCenterY + triangleSize,
        iconCenterX + triangleSize / 2,
        iconCenterY + triangleSize,
        bruceConfig.priColor
    );

    // Left Arcs
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius,
        2 * radius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );

    // Right Arcs
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius,
        2 * radius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
