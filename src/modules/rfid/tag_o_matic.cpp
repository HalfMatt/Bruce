/**
 * @file tag_o_matic.cpp
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read and Write RFID tags
 * @version 0.1
 * @date 2024-07-17
 */

#include "tag_o_matic.h"
#include "core/globals.h"
#include "core/mykeyboard.h"
#include "core/display.h"
#include "core/sd_functions.h"


TagOMatic::TagOMatic() {
    setup();
}

void TagOMatic::setup() {
    Wire.begin(GROVE_SDA, GROVE_SCL);
    mfrc522.PCD_Init();
    set_state(READ_MODE);
    delay(1000);
    return loop();
}

void TagOMatic::loop() {
    while(1) {
        if (checkEscPress()) {
            returnToMenu=true;
            break;
        }

        if (checkSelPress()) {
            select_state();
        }

        switch (current_state) {
            case READ_MODE:
                read_card();
                break;
            case LOAD_MODE:
                load_uid();
                break;
            case CLONE_MODE:
                clone_card();
                break;
            case SAVE_MODE:
                save_uid();
                break;
        }

    }
    Serial.println();
}

void TagOMatic::select_state() {
    options = {
        {"Read tag", [=]() { set_state(READ_MODE); }},
        {"Load file", [=]() { set_state(LOAD_MODE); }},
    };
    if (_read_uid) {
        options.push_back({"Clone tag", [=]() { set_state(CLONE_MODE); }});
        options.push_back({"Save file",  [=]() { set_state(SAVE_MODE); }});
    }
    delay(200);
    loopOptions(options);
}

void TagOMatic::set_state(RFID_State state) {
    current_state = state;
    display_banner();
    switch (state) {
        case READ_MODE:
        case LOAD_MODE:
            _read_uid = false;
            break;
        case CLONE_MODE:
            tft.println("New UID: " + printableUID.uid);
            tft.println("SAK: " + printableUID.sak);
            tft.println("");
            break;
        case SAVE_MODE:
            break;
    }
    delay(300);
}

void TagOMatic::cls() {
    tft.fillScreen(BGCOLOR);
    tft.setCursor(0, 0);
    tft.setTextColor(FGCOLOR, BGCOLOR);
}

void TagOMatic::display_banner() {
    cls();
    tft.setTextSize(FM);
    tft.println("TAG-O-MATIC");

    tft.setTextSize(FP);
    tft.println("     RFID2 I2C MFRC522");
    tft.println("     -----------------");

    tft.setTextSize(FM);
    switch (current_state) {
        case READ_MODE:
            tft.println("Read Mode");
            break;
        case LOAD_MODE:
            tft.println("Load Mode");
            break;
        case CLONE_MODE:
            tft.println("Clone Mode");
            break;
        case SAVE_MODE:
            tft.println("Save Mode");
            break;
    }

    tft.setTextSize(FP);
    tft.println("");
    tft.println("Press 'ENTER' to change mode.");
    tft.println("");
    tft.println("");
}

void TagOMatic::dump_card_details() {
	tft.println(printableUID.picc_type);
	tft.println("UID: " + printableUID.uid);
	tft.println("ATQA: " + printableUID.atqa);
	tft.println("SAK: " + printableUID.sak);
    if (!pageReadSuccess) {
	    tft.println("[!] Failed to read data blocks");
    }
}

bool TagOMatic::PICC_IsNewCardPresent() {
	byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);
	byte result = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);
	bool bl_result = (result == mfrc522.STATUS_OK || result == mfrc522.STATUS_COLLISION);
    if (bl_result) {
        printableUID.atqa = "";
        for (byte i = 0; i < bufferSize; i++) {
            printableUID.atqa += bufferATQA[i] < 0x10 ? " 0" : " ";
            printableUID.atqa += String(bufferATQA[i], HEX);
        }
        printableUID.atqa.trim();
        printableUID.atqa.toUpperCase();
    }
    return bl_result;
}

void TagOMatic::read_card() {
    if (!PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }
    display_banner();
    pageReadSuccess = read_data_blocks();
    format_data();
    dump_card_details();
    uid = mfrc522.uid;
    _read_uid = true;
    delay(1000);
}

void TagOMatic::clone_card() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }
    display_banner();

    if (mfrc522.MIFARE_SetUid(uid.uidByte, uid.size, true)) {
        displaySuccess("UID written successfully.");
    } else {
        displayError("Error writing UID to tag.");
    }

    mfrc522.PICC_HaltA();
    delay(2000);
    set_state(READ_MODE);
}

void TagOMatic::save_uid() {
    String uid_str = printableUID.uid;
    uid_str.replace(" ", "");
    String filename = keyboard(uid_str, 30, "File name:");

    display_banner();

    if (write_file(filename)) {
        displaySuccess("UID file saved.");
    }
    else {
        displayError("Error writing UID file.");
    }
    delay(2000);
    set_state(READ_MODE);
}

bool TagOMatic::write_file(String filename) {
    FS *fs;
    if(setupSdCard()) fs=&SD;
    else fs=&LittleFS;

    if (!(*fs).exists("/BruceRFID")) (*fs).mkdir("/BruceRFID");
    if ((*fs).exists("/BruceRFID/" + filename + ".rfid")) {
        int i = 1;
        filename += "_";
        while((*fs).exists("/BruceRFID/" + filename + String(i) + ".rfid")) i++;
        filename += String(i);
    }
    File file = SD.open("/BruceRFID/"+ filename + ".rfid", FILE_WRITE);

    if(!file) {
        return false;
    }

    file.println("Filetype: Bruce RFID File");
    file.println("Version 1");
    file.println("Device type: " + printableUID.picc_type);
    file.println("# UID, ATQA and SAK are common for all formats");
	file.println("UID: " + printableUID.uid);
	file.println("SAK: " + printableUID.sak);
	file.println("ATQA: " + printableUID.atqa);
    file.println("# Memory dump");
	file.println("Pages total: " + String(totalPages));
    if (!pageReadSuccess) file.println("Pages read: " + String(totalPages));
    file.print(strAllPages);

    file.close();
    delay(100);
    return true;
}

void TagOMatic::load_uid() {
    display_banner();

    if (load_from_file()) {
        parse_data();
        displaySuccess("UID loaded from file.");
        delay(2000);
        _read_uid = true;
        set_state(CLONE_MODE);
    }
    else {
        displayError("Error loading UID from file.");
        delay(2000);
        set_state(READ_MODE);
    }
}

bool TagOMatic::load_from_file() {
    cls();

    String filepath;
    File file;
    FS *fs;

    if(setupSdCard()) fs=&SD;
    else fs=&LittleFS;
    filepath = loopSD(*fs, true);
    file = fs->open(filepath, FILE_READ);

    if (!file) {
        return false;
    }

    String line;
    String strData;
    strAllPages = "";

    while (file.available()) {
        line = file.readStringUntil('\n');
        strData = line.substring(line.indexOf(":") + 1);
        strData.trim();
        if(line.startsWith("Device type:"))  printableUID.picc_type = strData;
        if(line.startsWith("UID:"))          printableUID.uid = strData;
        if(line.startsWith("SAK:"))          printableUID.sak = strData;
        if(line.startsWith("ATQA:"))         printableUID.atqa = strData;
        if(line.startsWith("Pages total:"))  totalPages = strData.toInt();
        if(line.startsWith("Pages read:"))   pageReadSuccess = false;
        if(line.startsWith("Page "))         strAllPages += line + "\n";
    }

    file.close();
    delay(100);
    return true;
}

void TagOMatic::format_data() {
	byte bcc = 0;

    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    printableUID.picc_type = mfrc522.PICC_GetTypeName(piccType);

	printableUID.sak = mfrc522.uid.sak < 0x10 ? "0" : "";
    printableUID.sak += String(mfrc522.uid.sak, HEX);
    printableUID.sak.toUpperCase();

	// UID
	printableUID.uid = "";
	for (byte i = 0; i < mfrc522.uid.size; i++) {
        printableUID.uid += mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ";
        printableUID.uid += String(mfrc522.uid.uidByte[i], HEX);
		bcc = bcc ^ mfrc522.uid.uidByte[i];
	}
    printableUID.uid.trim();
    printableUID.uid.toUpperCase();

	// BCC
	printableUID.bcc = bcc < 0x10 ? "0" : "";
    printableUID.bcc += String(bcc, HEX);
    printableUID.bcc.toUpperCase();

    // ATQA
    String atqaPart1 = printableUID.atqa.substring(0, 2);
    String atqaPart2 = printableUID.atqa.substring(3, 5);
    printableUID.atqa = atqaPart2 + " " + atqaPart1;
}

void TagOMatic::parse_data() {
    String strUID = printableUID.uid;
    strUID.trim();
    strUID.replace(" ", "");
    uid.size = strUID.length() / 2;
    for (size_t i = 0; i < strUID.length(); i += 2) {
        uid.uidByte[i / 2] = strtoul(strUID.substring(i, i + 2).c_str(), NULL, 16);
    }

    printableUID.sak.trim();
    uid.sak = strtoul(printableUID.sak.c_str(), NULL, 16);

    // int pageIndex = line.substring(5, line.indexOf(":")).toInt();
}

bool TagOMatic::read_data_blocks() {
    totalPages = 0;
    bool readSuccess = false;
	MFRC522::MIFARE_Key key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    strAllPages = "";

	switch (piccType) {
		case MFRC522::PICC_TYPE_MIFARE_MINI:
		case MFRC522::PICC_TYPE_MIFARE_1K:
		case MFRC522::PICC_TYPE_MIFARE_4K:
			readSuccess = read_mifare_classic_data_blocks(piccType, &key);
			break;

		case MFRC522::PICC_TYPE_MIFARE_UL:
			readSuccess = read_mifare_ultralight_data_blocks();
            totalPages = (readSuccess && totalPages > 0) ? totalPages - 1 : totalPages;
			break;

		default:
			break;
	}

	mfrc522.PICC_HaltA();
    return readSuccess;
}

bool TagOMatic::read_mifare_classic_data_blocks(byte piccType, MFRC522::MIFARE_Key *key) {
	byte no_of_sectors = 0;
    bool sectorReadSuccess;

	switch (piccType) {
		case MFRC522::PICC_TYPE_MIFARE_MINI:
			no_of_sectors = 5;
			break;

		case MFRC522::PICC_TYPE_MIFARE_1K:
			no_of_sectors = 16;
			break;

		case MFRC522::PICC_TYPE_MIFARE_4K:
			no_of_sectors = 40;
			break;

		default: // Should not happen. Ignore.
			break;
	}

	if (no_of_sectors) {
		for (int8_t i = 0; i < no_of_sectors; i++) {
			sectorReadSuccess = read_mifare_classic_data_sector(key, i);
            if (!sectorReadSuccess) break;
		}
	}
	mfrc522.PICC_HaltA();
	mfrc522.PCD_StopCrypto1();
    return sectorReadSuccess;
}

bool TagOMatic::read_mifare_classic_data_sector(MFRC522::MIFARE_Key *key, byte sector) {
	byte status;
	byte firstBlock;
	byte no_of_blocks;
	bool isSectorTrailer;
	byte c1, c2, c3;
	byte c1_, c2_, c3_;
	bool invertedError;
	byte g[4];
	byte group;
	bool firstInGroup;

	if (sector < 32) {
		no_of_blocks = 4;
		firstBlock = sector * no_of_blocks;
	}
	else if (sector < 40) {
		no_of_blocks = 16;
		firstBlock = 128 + (sector - 32) * no_of_blocks;
	}
	else {
		return false;
	}

	byte byteCount;
	byte buffer[18];
	byte blockAddr;
	isSectorTrailer = true;
	String strPage;

	for (int8_t blockOffset = 0; blockOffset < no_of_blocks; blockOffset++) {
        strPage = "";
		blockAddr = firstBlock + blockOffset;
		if (isSectorTrailer) {
			status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, firstBlock, key, &mfrc522.uid);
			if (status != MFRC522::STATUS_OK) {
				return false;
			}
		}
		byteCount = sizeof(buffer);
		status = mfrc522.MIFARE_Read(blockAddr, buffer, &byteCount);
		if (status != MFRC522::STATUS_OK) {
			return false;
		}
		for (byte index = 0; index < 16; index++) {
            strPage += buffer[index] < 0x10 ? F(" 0") : F(" ");
			strPage += String(buffer[index], HEX);
		}
		if (isSectorTrailer) {
			c1  = buffer[7] >> 4;
			c2  = buffer[8] & 0xF;
			c3  = buffer[8] >> 4;
			c1_ = buffer[6] & 0xF;
			c2_ = buffer[6] >> 4;
			c3_ = buffer[7] & 0xF;
			invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
			g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
			g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
			g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
			g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
			isSectorTrailer = false;
		}

		if (no_of_blocks == 4) {
			group = blockOffset;
			firstInGroup = true;
		}
		else {
			group = blockOffset / 5;
			firstInGroup = (group == 3) || (group != (blockOffset + 1) / 5);
		}

        strPage.trim();
        strPage.toUpperCase();

		strAllPages += "Page " + String(totalPages) + ": " + strPage + "\n";
        totalPages++;
	}

	return true;
}

bool TagOMatic::read_mifare_ultralight_data_blocks() {
	byte status;
	byte byteCount;
	byte buffer[18];
	byte i;
	String strPage = "";

	for (byte page = 0; page < 256; page +=4) {
		byteCount = sizeof(buffer);
		status = mfrc522.MIFARE_Read(page, buffer, &byteCount);
		if (status != MFRC522::STATUS_OK) {
            return status == MFRC522::STATUS_MIFARE_NACK;
		}
		for (byte offset = 0; offset < 4; offset++) {
            strPage = "";
			i = page + offset;
			for (byte index = 0; index < 4; index++) {
				i = 4 * offset + index;
	            strPage += buffer[i] < 0x10 ? F(" 0") : F(" ");
				strPage += String(buffer[i], HEX);
			}
            strPage.trim();
            strPage.toUpperCase();

		    strAllPages += "Page " + String(totalPages) + ": " + strPage + "\n";
            totalPages++;
		}
	}

    return true;
}
