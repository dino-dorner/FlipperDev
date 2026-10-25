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

/* stock overview scene */

void stock_app_scene_on_enter_stock_overview(void* context) {
    FURI_LOG_T(TAG, "stock_app_scene_on_enter_stock_overview");

    StockTrackerApp* app = context;

    app->selected_stock_index = 0;

    for (size_t i = 0; i < app->ticker_count; i++){
        app->stock_loaded[i] = false;
    }

    view_dispatcher_switch_to_view(
        app->view_dispatcher, 
        stockTrackerView_Overview);
    
    if (app->ticker_count > 0){
        stock_app_request_all_stocks_async(app);
    }
}

bool stock_app_scene_on_event_stock_overview(void* context, SceneManagerEvent event) {
    StockTrackerApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        return false;
    }

    if (event.event == stockTracker_NextStock || 
        event.event == stockTracker_PreviousStock) {
            with_view_model(
                app->ticker_overview,
                StockOverviewModel* model,
                {
                    size_t count = app->ticker_count;

                    if(count > 0) {
                        if(event.event == stockTracker_NextStock){
                            model->selected_index =
                                (model->selected_index + 1) % count;
                        } else {
                            model->selected_index = (model->selected_index + count - 1) % count;
                        }
                    }
                },
                true
            );

            return true;
        }

    if (event.event == stockTracker_stockResponseReceived) {
        view_commit_model(app->ticker_overview, true);
    }

    return false; // don't handle any events
}


void stock_app_display_stock(void* context){
    StockTrackerApp* app = context;
    widget_add
    for (size_t i = 1)
}

void stock_app_scene_on_exit_stock_overview(void* context) {
    FURI_LOG_T(TAG, "stock_app_scene_on_exit_popup_two");
    StockTrackerApp* app = context;
    widget_reset(app->ticker_overview);
}


void stock_app_overview_draw(Canvas* canvas, void* context) {
    StockTrackerApp* app = context;

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if (app->ticker_count == 0){
        canvas_draw_str(canvas, 10, 30, "No stocks added");
        return;
    }

    size_t i = app->selected_stock_index;

    if(i >= app->ticker_count) return;

    canvas_draw_str(canvas, 45, 15, app->tickers[i]);

    char price_text[32];

    if (app->stock_loaded[i]) {
        snprintf(
            price_text,
            sizeof(price_text),
            "$%.2f",
            (double)app->stock_prices[i]
        );
    } else {
        snprintf(price_text, sizeof(price_text), "Loading...");
    }

    canvas_draw_str(canvas, 30, 35, price_text);

    canvas_draw_str(canvas, 2, 35, "<");
    canvas_draw_str(canvas, 120, 35, ">");

    char page[20];

    snprintf(
        page,
        sizeof(page),
        "%u/%u",
        (unsigned)(i+1),
        (unsigned)app->ticker_count
    );

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 55, 60, page);

}


bool stock_app_overview_input(InputEvent* event, void* context) {
    StockTrackerApp* app = context;

    if(event->type != InputTypeShort) {
        return false;
    }

    if(event->key == InputKeyRight){
        view_dispatcher_send_custom_event(
            app->view_dispatcher,
            stockTracker_NextStock
        );
        return true;
    }

    if(event->key == InputKeyLeft) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher,
            stockTracker_PreviousStock
        );
        return true;
    }

    return false;
}