#include <furi.h>
#include <gui/gui.h>
#include <gui/icon_i.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/menu.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <storage/storage.h>
#include <string.h>
#include "stock_app.h"

//This function saves the tickers that were added through the app.
bool stock_app_save_tickers(StockTrackerApp* app) {

    //Opens the storage pathway using the DEFINED pathway.
    Storage* storage = furi_record_open(RECORD_STORAGE);

    //Iniitializes a storage instance.
    File* file = storage_file_alloc(storage);

    //Success bool for opening (fopen basically!)
    bool success = storage_file_open(
        file,
        TICKER_FILE,
        FSAM_WRITE,
        FSOM_CREATE_ALWAYS
    );


    //If a successful opening.
    if(success) {
        for(size_t i = 0; i < app->ticker_count; i++) {

            const char* ticker = app->tickers[i];
            size_t length = strlen(ticker);

            // Write ticker string
            if(storage_file_write(file, ticker, length) != length) {
                success = false;
                break;
            }

            // Write newline separator
            if(storage_file_write(file, "\n", 1) != 1) {
                success = false;
                break;
            }
        }

        if(success) {
            //Success file store.
            success = storage_file_sync(file);
        }
    }

    if(!success) {
        FURI_LOG_E(TAG, "Failed to save tickers");
    }

    //Close and free file pointers.
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    //Return the success.
    return success;
}

bool stock_app_load_tickers(StockTrackerApp* app) {
    //Opens storage pointer
    Storage* storage = furi_record_open(RECORD_STORAGE);

    //Opens file pointer
    File* file = storage_file_alloc(storage);

    //reset ticker to 0.
    app->ticker_count = 0;

    //open file.
    bool opened = storage_file_open(
        file,
        TICKER_FILE,
        FSAM_READ,
        FSOM_OPEN_EXISTING
    );

    if (!opened){
        //free pointers if not opened.
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    char buffer[TICKER_LENGTH];
    size_t pos = 0;
    bool overflow = false;
    char c;

    while (app->ticker_count < MAX_TICKERS &&
            storage_file_read(file, &c, 1) == 1){
        if (c == '\n'){

            //No overflow and pos greater than 0, insert null and load buffer.
            if(pos > 0 && !overflow) {
                //Insert null character for strings.
                buffer[pos] = '\0';
                
                //Copy the buffer into the array of stocks.
                strcpy(
                    app->tickers[app->ticker_count],
                    buffer
                );
                
                //increment the app ticker count.
                app->ticker_count++;
            }
            
            //Reset overflow and set position back to 0.
            pos = 0;
            overflow = false;
        }
        else if (c != '\r'){
            //Check position length (with buffer)
            if (pos < TICKER_LENGTH-1){
                buffer[pos++] = c;
            }
            //Set overflow to true.
            else {
                overflow = true;
            }
        }
    }

    //Account for the last line (no new line inserted)
    if (pos > 0 && !overflow &&
        app->ticker_count < MAX_TICKERS){

        //Load buffer with the null character.
        buffer[pos] = '\0';
        strcpy(app->tickers[app->ticker_count], buffer);
        app->ticker_count++;
    }

    //Close pointers.
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Loaded %u tickers", (size_t)app->ticker_count);
    return true;
}