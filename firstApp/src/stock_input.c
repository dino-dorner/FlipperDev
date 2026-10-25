/* stock select scene*/
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

void stock_app_scene_on_enter_stock_select(void* context) {
    FURI_LOG_T(TAG, "stock_app_scene_on_enter_stock_select");

    StockTrackerApp* app = context;

    text_input_reset(app->ticker_select);
    text_input_set_header_text(app->ticker_select, "Enter Stock Ticker");
    text_input_set_result_callback(app->ticker_select, stock_app_ticker_input_callback,
    app,
    app->selected_ticker,
    sizeof(app->selected_ticker),
    false);
    view_dispatcher_switch_to_view(app->view_dispatcher, stockTrackerView_Select);
}

bool stock_app_scene_on_event_stock_select(void* context, SceneManagerEvent event){
    StockTrackerApp* app = context;

    if (event.type == SceneManagerEventTypeCustom && 
        event.event == stockTracker_TickerEntered){
            
            if (stock_app_add_ticker(app, app->selected_ticker)){

                if(!stock_app_save_tickers(app)){
                    FURI_LOG_E(TAG, "Failed to save added ticker");
                }

                scene_manager_next_scene(
                    app->scene_manager,
                    stockTrackerScene_MainMenu
                );
            }

            return true;
        }

    return false;
}

bool stock_app_add_ticker(
    StockTrackerApp* app,
    const char* ticker
) {
    if (app->ticker_count >= MAX_TICKERS){
        return false;
    }

    size_t length = strlen(ticker);

    if (length == 0 || length >= TICKER_LENGTH){
        return false;
    }

    for (size_t i = 0; i < app->ticker_count; i++){
        if(strcmp(app->tickers[i], ticker) == 0){
            return false;
        }
    }

    strcpy(app->tickers[app->ticker_count], ticker);
    app->ticker_count++;

    return true;
}

void stock_app_scene_on_exit_stock_select(void* context) {
    FURI_LOG_T(TAG, "test_app_scene_on_exit_popup_one");
    StockTrackerApp* app = context;
    text_input_reset(app->ticker_select);
}

void stock_app_ticker_input_callback(void * context){
    StockTrackerApp* app = context;

    FURI_LOG_I(TAG, "Ticker selected: %s", app->selected_ticker);

    view_dispatcher_send_custom_event(
        app->view_dispatcher,
        stockTracker_TickerEntered
    );
}