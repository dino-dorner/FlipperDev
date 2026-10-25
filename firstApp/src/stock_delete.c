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


bool stock_app_remove_ticker(StockTrackerApp* app, size_t index){
    if (index >= app->ticker_count){
        return false;
    }

    //Copy each. Basically like dequeueing.
    for (size_t i = index; i < app->ticker_count - 1; i++){
        strcpy(app->tickers[i], app->tickers[i+1]);
    }

    //Decrement the ticker_count.
    app->ticker_count--;
    
    //Set to null.
    app->tickers[app->ticker_count][0] = '\0';

    return true;
}

void stock_app_scene_on_enter_delete_ticker(void* context){
    
    FURI_LOG_T(TAG, "stock_app_scene_on_enter_delete_ticker");

    StockTrackerApp* app = context;

    submenu_reset(app->ticker_delete);

    char header[] = "Current Stocks";

    submenu_set_header(app->ticker_delete, header);
    for (size_t i = 0; i < app->ticker_count; i++){
        submenu_add_item(
            app->ticker_delete, 
            app->tickers[i],
            i,
            stock_app_delete_callback,
            app);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, stockTrackerView_TickerDelete);
}

bool stock_app_scene_on_event_delete_ticker(void* context, SceneManagerEvent event){
    StockTrackerApp* app = context;

    if (event.type == SceneManagerEventTypeCustom && 
        event.event == stockTracker_DeleteTickerSelected){
            
            if (stock_app_remove_ticker(app, app->index)){

                stock_app_save_tickers(app);

                submenu_reset(app->ticker_delete);
                submenu_set_header(app->ticker_delete, "Current Stocks");


                // Rebuild submenu using updated array
                for(size_t i = 0; i < app->ticker_count; i++) {
                    submenu_add_item(
                        app->ticker_delete,
                        app->tickers[i],
                        i,
                        stock_app_delete_callback,
                        app
                    );
                }
            }

            return true;
        }

    return false;
}

void stock_app_scene_on_exit_delete_ticker(void* context) {
    FURI_LOG_T(TAG, "stock_app_scene_on_exit_delete_ticker");
    StockTrackerApp* app = context;
    submenu_reset(app->ticker_delete);
}


void stock_app_delete_callback(void* context, uint32_t index){
    StockTrackerApp* app = context;

    app->index = index;

    view_dispatcher_send_custom_event(
        app->view_dispatcher,
        stockTracker_DeleteTickerSelected
    );
}