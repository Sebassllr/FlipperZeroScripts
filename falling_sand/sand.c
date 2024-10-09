#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <gui/view.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <dolphin/dolphin.h>


#define TAG "Falling_Sand"

#define FLIPPER_LCD_WIDTH 128
#define FLIPPER_LCD_HEIGHT 64
#define MAX_SPEED 3


#define RECORD_NOTIFICATION "notification"
#define MAX_DROPS 5
#define Y_MARGIN 1
#define MAX_SAND 10
typedef struct NotificationApp NotificationApp;

typedef enum { EventTypeTick, EventTypeKey } EventType;


typedef struct {
    EventType type;
    InputEvent input;
} GameEvent;

typedef struct {
    int y;
    int x;
    bool landed;
} SandState;

typedef struct {
    FuriMutex* mutex;
    bool initialDraw;
    bool gameStarted;
    int speed;
    int drop;
    int y;
    int x;
    int sand[MAX_SAND];
    SandState sandState[FLIPPER_LCD_HEIGHT + 1][FLIPPER_LCD_WIDTH + 1];
    NotificationApp* notify;
} FallingSandState;

static void falling_sand_state_init(FallingSandState* falling_sand_state) {
    falling_sand_state->notify = furi_record_open(RECORD_NOTIFICATION);
    falling_sand_state->speed = 1;
    falling_sand_state->sand[0] = 10;

    falling_sand_state->y = 10;
    falling_sand_state->x = FLIPPER_LCD_HEIGHT;
    falling_sand_state->initialDraw = false;
    falling_sand_state->gameStarted = false;

    for(int x = 0; x <= FLIPPER_LCD_HEIGHT; x++) {
        for(int y = 0; y <= FLIPPER_LCD_WIDTH; y++) {
            SandState state = {x, y, false};
            falling_sand_state->sandState[x][y] = state;
        }
    }
}

void drop_sand(Canvas* canvas, FallingSandState* falling_sand_state) {
    canvas_draw_dot(canvas, falling_sand_state->x, falling_sand_state->y);
}

static void falling_sand_draw_callback(Canvas* const canvas, void* ctx) {
    /*furi_assert(ctx);
    FallingSandState* falling_sand_state = ctx;
    furi_mutex_acquire(falling_sand_state->mutex, FuriWaitForever);
    canvas_draw_dot(canvas, 10 * 1, falling_sand_state->y);

    if(!falling_sand_state->initialDraw) {
        falling_sand_state->initialDraw = true;
        canvas_set_font(canvas, FontSecondary);

        //reset_level(canvas, falling_sand_state);
    } else {
        falling_sand_state->drop = falling_sand_state->y + falling_sand_state->drop + falling_sand_state->speed;

        //if(falling_sand_state->drop < FLIPPER_LCD_WIDTH) {
            drop_sand(canvas, falling_sand_state);
        //}
    }

    furi_mutex_release(falling_sand_state->mutex);*/

    furi_assert(ctx);
    FallingSandState* falling_sand_state = ctx;
    furi_mutex_acquire(falling_sand_state->mutex, FuriWaitForever);
    
    for(int x = 0; x < FLIPPER_LCD_HEIGHT; x++) {
        for(int y = 0; y < FLIPPER_LCD_WIDTH; y++) {
            bool landed = falling_sand_state->sandState[x][y].landed;
            if(landed) {
                canvas_draw_dot(canvas, y, x);
            }
        }
    }

    // Simple draw of the dot
    canvas_draw_dot(canvas, falling_sand_state->y, falling_sand_state->x);
    
    furi_mutex_release(falling_sand_state->mutex);
}

static void falling_sand_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    GameEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void falling_sand_update_timer_callback(FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    GameEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}


static void calculate_x_y_position(FallingSandState* falling_sand_state, int x, int y, bool left) {
    

    if(y > FLIPPER_LCD_WIDTH - Y_MARGIN) {
        y = FLIPPER_LCD_WIDTH - Y_MARGIN;
    }

    /*
    if(x >= FLIPPER_LCD_HEIGHT) {
        x = FLIPPER_LCD_HEIGHT;
    } else if(x <= 0) {
        x = 1;
    }*/

    if(!falling_sand_state->sandState[x][y].landed) {
        falling_sand_state->sandState[x][y].landed = true;
        falling_sand_state->y = 10;
        return;
    }

    if(x > 0 && x < FLIPPER_LCD_HEIGHT) {
        if(falling_sand_state->sandState[x - 1][y].landed && falling_sand_state->sandState[x + 1][y].landed) {
            calculate_x_y_position(falling_sand_state, x, y - 1, left);
            return;
        }

        if(left && !falling_sand_state->sandState[x - 1][y].landed) {
            calculate_x_y_position(falling_sand_state, x - 1, y + 1, left);
            return;
        } 
    
        if(!left && !falling_sand_state->sandState[x + 1][y].landed) {
            calculate_x_y_position(falling_sand_state, x + 1, y + 1, left);
            return;
        } 
    }

    calculate_x_y_position(falling_sand_state, left? x - 1 : x + 1, y, left);
    return;
}

static void set_x_y_falling_coordinates(FallingSandState* falling_sand_state) {

    int min = 0;
    int max = 10;
    int random_number = (rand() % (max - min + 1)) + min;
    bool left = random_number > 5;

    if(falling_sand_state->x == 0) {
        left = false;
    } else if(falling_sand_state->x == FLIPPER_LCD_HEIGHT){
        left = true;
    }

    calculate_x_y_position(falling_sand_state, falling_sand_state->x, falling_sand_state->y, left);
}

int32_t game_falling_sand(void* p) {
    UNUSED(p);
    int32_t return_code = 0;
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(GameEvent));
    
    FallingSandState* falling_sand_state = malloc(sizeof(FallingSandState));
    falling_sand_state_init(falling_sand_state);

    falling_sand_state->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    if(!falling_sand_state->mutex) {
        FURI_LOG_E(TAG, "Cannot create mutex\r\n");
        return_code = 255;
        goto free_and_exit;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, falling_sand_draw_callback, falling_sand_state);
    view_port_input_callback_set(view_port, falling_sand_input_callback, event_queue);

    FuriTimer* timer =
        furi_timer_alloc(falling_sand_update_timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency() / 22);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Call dolphin deed on game start
    dolphin_deed(DolphinDeedPluginGameStart);

    GameEvent event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);
        furi_mutex_acquire(falling_sand_state->mutex, FuriWaitForever);
        if(event_status == FuriStatusOk) {
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress || event.input.type == InputTypeLong ||
                   event.input.type == InputTypeRepeat) {
                    switch(event.input.key) {
                        case InputKeyBack:
                            processing = false;
                            break;
                        case InputKeyUp:
                            if(falling_sand_state->x > 0) {
                                falling_sand_state->x = falling_sand_state->x - 1;
                            }
                            
                            break;
                        case InputKeyDown:
                            if(falling_sand_state->x < FLIPPER_LCD_HEIGHT) {
                                falling_sand_state->x = falling_sand_state->x + 1;
                            }
                            
                            break;
                        default:
                            break;
                    }
                }
            }
        }


        if(falling_sand_state->sandState[falling_sand_state->x][falling_sand_state->y].landed) {
            set_x_y_falling_coordinates(falling_sand_state);
        } else if (falling_sand_state->y < FLIPPER_LCD_WIDTH - Y_MARGIN) {
            falling_sand_state->y = falling_sand_state->y + 1;
        } else {
            set_x_y_falling_coordinates(falling_sand_state);
            falling_sand_state->y = 10;
        }
                            
        furi_mutex_release(falling_sand_state->mutex);
        view_port_update(view_port);
        furi_delay_ms(20);
    }

    furi_timer_free(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    view_port_free(view_port);
    furi_mutex_free(falling_sand_state->mutex);
free_and_exit:
    free(falling_sand_state);
    furi_message_queue_free(event_queue);

    return return_code;
}