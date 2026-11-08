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

//These are all function pointers to their handlers.
/** collection of all scene on_enter handlers - in the same order as their enum */
void (*const stock_ticker_scene_on_enter_handlers[])(void*) = {
    stock_app_scene_on_enter_main_menu,
    stock_app_scene_on_enter_stock_select,
    stock_app_scene_on_enter_stock_overview,
    stock_app_scene_on_enter_delete_ticker,
    stock_app_scene_on_enter_wifi
};

/** collection of all scene on event handlers - in the same order as their enum */
bool (*const stock_ticker_scene_on_event_handlers[])(void*, SceneManagerEvent) = {
    stock_app_scene_on_event_main_menu,
    stock_app_scene_on_event_stock_select,
    stock_app_scene_on_event_stock_overview,
    stock_app_scene_on_event_delete_ticker,
    stock_app_scene_on_event_wifi
};

/** collection of all scene on exit handlers - in the same order as their enum */
void (*const stock_ticker_scene_on_exit_handlers[])(void*) = {
    stock_app_scene_on_exit_main_menu,
    stock_app_scene_on_exit_stock_select,
    stock_app_scene_on_exit_stock_overview,
    stock_app_scene_on_exit_delete_ticker,
    stock_app_scene_on_event_wifi
};

/** collection of all on_enter, on_event, on_exit handlers */
const SceneManagerHandlers stock_ticker_scene_event_handlers = {
    .on_enter_handlers = stock_ticker_scene_on_enter_handlers,
    .on_event_handlers = stock_ticker_scene_on_event_handlers,
    .on_exit_handlers = stock_ticker_scene_on_exit_handlers,
    .scene_num = stockTrackerScene_Count};

/** entrypoint */
int32_t stock_tracker_app(void* p) {
    UNUSED(p);
    stock_app_set_log_level();

    // create the app context struct, scene manager, and view dispatcher
    FURI_LOG_I(TAG, "stock app starting...");
    StockTrackerApp* app = stock_app_init();

    // set the scene and launch the main loop
    Gui* gui = furi_record_open(RECORD_GUI);

    //Attaches view dispatcher to gui.
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    

    //Transitions to the MainMenu scene. ENUM is index 0. 
    //Scene manager uses this to select which test_app_scene_event_handlers to
    //do. Each have a set of enter/event/enter handlers.
    scene_manager_next_scene(app->scene_manager, stockTrackerScene_MainMenu);
    FURI_LOG_D(TAG, "Starting dispatcher...");

    //Runs the view dispatcher.
    view_dispatcher_run(app->view_dispatcher);

    // free all memory
    FURI_LOG_I(TAG, "Test app finishing...");

    stock_app_save_tickers(app);
    furi_record_close(RECORD_GUI);
    stock_app_free(app);
    return 0;
}

/** go to trace log level in the dev environment
 * 
 *  Basically just sets the logs for the application.
 */
void stock_app_set_log_level() {
    //DEFINE for preprocessor.
#ifdef FURI_DEBUG
    furi_log_set_level(FuriLogLevelTrace);
#else
    furi_log_set_level(FuriLogLevelInfo);
#endif
}


/** initialise app data, scene manager, and view dispatcher 
 * 
 * Returns a pointer to the struct of pointers to a bunch
 * of handlers.
*/
StockTrackerApp* stock_app_init() {
    //Logging macro used to trace diagnostic messages on the 
    // flipper zero.
    FURI_LOG_T(TAG, "stock_app_init");

    //Allocates block of memory dynamically.
    StockTrackerApp* app = calloc(1, sizeof(StockTrackerApp));
    furi_check(app);

    stock_app_load_tickers(app);


    //Initializes the scenes. These basically allocate memory.
    stock_app_scene_manager_init(app);
    stock_app_view_dispatcher_init(app);
    return app;
}


/** initialise the scene manager with all handlers */
void stock_app_scene_manager_init(StockTrackerApp* app) {
    //Logs.
    FURI_LOG_T(TAG, "stock_app_scene_manager_init");

    //App (which is a pointer) accessing its scene_manage value. Allocates memory
    // for the scenemanager. Second is just the app context.
    //This is from the flipper library. 
    app->scene_manager = scene_manager_alloc(&stock_ticker_scene_event_handlers, app);
}


/** initialise the views, and initialise the view dispatcher with all views */
void stock_app_view_dispatcher_init(StockTrackerApp* app) {
    FURI_LOG_T(TAG, "test_app_view_dispatcher_init");

    // View dispatcher manages multiple screens, user inputs, and visual states.
    //This calls a function from the flipper zero library.
    app->view_dispatcher = view_dispatcher_alloc();

    // allocates each view
    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init allocating views");
    app->menu = menu_alloc();
    app->ticker_select = text_input_alloc();
    app->ticker_overview = view_alloc();
    app->wifi = text_input_alloc();

    view_allocate_model(
        app->ticker_overview,
        ViewModelTypeLocking,
        sizeof(StockTrackerApp*)
    );

    app->ticker_delete = submenu_alloc();

    view_set_context(app->ticker_overview, app);

    view_set_draw_callback(
        app->ticker_overview,
        stock_app_overview_draw
    );

    view_set_input_callback(
        app->ticker_overview,
        stock_app_overview_input
    );

    with_view_model(
        app->ticker_overview,
        StockTrackerApp** model,
        {
            *model = app;
        },
        false
    );

    // assign callback that pass events from views to the scene manager
    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init setting callbacks");
    //Allows one to manipulate application state when an event occurs.
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    //Sets events callbacks.
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, stock_app_scene_manager_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, stock_app_scene_manager_navigation_event_callback);

    // add views to the dispatcher, indexed by their enum value
    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init adding view menu");
    view_dispatcher_add_view(app->view_dispatcher, stockTrackerView_Main, menu_get_view(app->menu));

    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init adding view stock ticker select");
    view_dispatcher_add_view(app->view_dispatcher, stockTrackerView_Select, text_input_get_view(app->ticker_select));

    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init adding view stock ticker overview");
    view_dispatcher_add_view(app->view_dispatcher, stockTrackerView_Overview, app->ticker_overview);

    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init adding view stock ticker delete");
    view_dispatcher_add_view(app->view_dispatcher, stockTrackerView_TickerDelete, submenu_get_view(app->ticker_delete));

    FURI_LOG_D(TAG, "stock_app_view_dispatcher_init adding wifi");
    view_dispatcher_add_view(app->view_dispatcher, stockTrackerView_Wifi, text_input_get_view(app->wifi));
}

/** custom event handler - passes the event to the scene manager */
//Helper method to bridge between the view dispatcher method above, and scene_manager handle.
bool stock_app_scene_manager_custom_event_callback(void* context, uint32_t custom_event) {
    FURI_LOG_T(TAG, "stock_app_scene_manager_custom_event_callback");
    furi_assert(context);

    //UNECESSARY! Pass by reference anyways? confused unless its just casting. #TODO.
    StockTrackerApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, custom_event);
}

/** navigation event handler - passes the event to the scene manager */
//Helper method to bridge between the view dispatcher method above, and scene_manager handle.
bool stock_app_scene_manager_navigation_event_callback(void* context) {
    FURI_LOG_T(TAG, "test_app_scene_manager_navigation_event_callback");
    furi_assert(context);
    StockTrackerApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

/** free all app data, scene manager, and view dispatcher. Prevents memory leaks. */
void stock_app_free(StockTrackerApp *app) {
    FURI_LOG_T(TAG, "stock_app_free");
    scene_manager_free(app->scene_manager);
    view_dispatcher_remove_view(app->view_dispatcher, stockTrackerView_Main);
    view_dispatcher_remove_view(app->view_dispatcher, stockTrackerView_Select);
    view_dispatcher_remove_view(app->view_dispatcher, stockTrackerView_Overview);
    view_dispatcher_remove_view(app->view_dispatcher, stockTrackerView_TickerDelete);
    view_dispatcher_remove_view(app->view_dispatcher, stockTrackerView_Wifi)
    view_dispatcher_free(app->view_dispatcher);
    menu_free(app->menu);
    text_input_free(app->ticker_select);
    text_input_free(app->wifi);
    view_free(app->ticker_overview);
    submenu_free(app->ticker_delete);
    free(app);
}
