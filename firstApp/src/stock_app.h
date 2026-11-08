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

#define MAX_TICKERS 32
#define TICKER_LENGTH 16
#define TAG "stock-tracker"

//Path to the App data for storing the ticker info.
#define TICKER_FILE APP_DATA_PATH("tickers.txt")

/** the app context struct */
typedef struct {

    //Scene and View managers (basically application infrastructure)
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;

    //UI modules
    Menu* menu;
    TextInput* ticker_select;
    TextInput* wifi;
    View* ticker_overview;
    Submenu* ticker_delete;

    //Ticker delete index;
    size_t index;

    //Stored values for selected stock
    char selected_ticker[TICKER_LENGTH];
    float stock_prices[MAX_TICKERS];
    float stock_changes[MAX_TICKERS];
    bool stock_loaded[MAX_TICKERS];
    char tickers[MAX_TICKERS][TICKER_LENGTH];

    //for wifi
    char wifi_ssid[33];
    char wifi_password[65];

    bool wifi_configured;
    bool wifi_connected;

    //for the custom view.
    size_t selected_index;
    size_t ticker_count;

    float stock_price;
    float stock_change_percent;
} StockTrackerApp;

// Main menu scene
void stock_app_scene_on_enter_main_menu(void* context);
bool stock_app_scene_on_event_main_menu(void* context, SceneManagerEvent event);
void stock_app_scene_on_exit_main_menu(void* context);

// Ticker selection scene
void stock_app_scene_on_enter_stock_select(void* context);
bool stock_app_scene_on_event_stock_select(void* context, SceneManagerEvent event);
void stock_app_scene_on_exit_stock_select(void* context);

//Ticker deletion scene
void stock_app_scene_on_enter_delete_ticker(void* context);
bool stock_app_scene_on_event_delete_ticker(void* context, SceneManagerEvent event);
void stock_app_scene_on_exit_delete_ticker(void* context);

// Ticker overview scene
void stock_app_scene_on_enter_stock_overview(void* context);
bool stock_app_scene_on_event_stock_overview(void* context, SceneManagerEvent event);
void stock_app_scene_on_exit_stock_overview(void* context);

//Wifi scene
void stock_app_scene_on_enter_wifi(void* context);
bool stock_app_scene_on_event_wifi(void* context, SceneManagerEvent event);
void stock_app_scene_on_exit_wifi(void* context);

// Application functions
void stock_app_set_log_level(void);
StockTrackerApp* stock_app_init(void);
void stock_app_scene_manager_init(StockTrackerApp* app);
void stock_app_view_dispatcher_init(StockTrackerApp* app);
void stock_app_free(StockTrackerApp* app);

// Dispatcher callbacks
bool stock_app_scene_manager_custom_event_callback(void* context, uint32_t event);
bool stock_app_scene_manager_navigation_event_callback(void* context);

// Menu callback
void stock_app_menu_callback_main_menu(void* context, uint32_t index);

// Delete callback
void stock_app_delete_callback(void* context, uint32_t index);

// Add ticker callback
void stock_app_ticker_input_callback(void* context);

//Add and remove ticker functions
bool stock_app_add_ticker(
    StockTrackerApp* app,
    const char* ticker
);

bool stock_app_remove_ticker(
    StockTrackerApp* app,
    size_t index
);

//Save and load tickers
bool stock_app_save_tickers(StockTrackerApp* app);
bool stock_app_load_tickers(StockTrackerApp* app);


//UI includes views, which are visual components -- menu/popup.

//Scenes are above views. Handle views, essentially.

/** ids for all scenes used by the app */
typedef enum {
    //Aligns with the indexes in on_event/on_enter/on_exit.
    stockTrackerScene_MainMenu,
    stockTrackerScene_TickerSelect,
    stockTrackerScene_TickerScreen,
    stockTrackerScene_TickerDelete,
    stockTrackerScene_Wifi,
    //Easy way to count the amount of scenes
    //that auto increments due to enum.
    stockTrackerScene_Count
} StockTrackerScene;

/** ids for the 2 types of view used by the app */
typedef enum { stockTrackerView_Main, stockTrackerView_Select,
    stockTrackerView_Overview, stockTrackerView_TickerDelete, stockTrackerView_Wifi,
     stockTrackerView_Count} StockTrackerView;

/** all custom events */
typedef enum { stockTracker_ShowTickerSelect, stockTracker_ShowTickerOverview, 
    stockTracker_TickerEntered, stockTracker_DeleteTickerSelected, 
    stockTracker_ShowTickerDelete, stockTracker_StockResponseReceived,
    stockTracker_NextStock, stockTracker_PreviousStock
} StockTrackerEvent;

/* main menu scene */

/** indices for menu items */
typedef enum { Menu_TickerSelect, Menu_TickerOverview, Menu_TickerDelete } MenuSelection;

