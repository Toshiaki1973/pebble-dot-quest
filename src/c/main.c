#include <pebble.h>

// RPGメッセージウィンドウ風ウォッチフェイス(flint 144x168 白黒)
// 自作フォント Pebble Dot 16x20: 1文字16x20、送り18px。40pxは2倍

#define CHAR_ADV 18
#define TEXT_PAD_X 9
#define BIG_DIGIT_W 32   // 40pxの数字インク幅
#define BIG_GAP 2
#define BIG_COLON_INK_X 12  // 40pxコロンの箱内インク開始位置
#define BIG_COLON_INK_W 8
#define MSG_INTERVAL_MS 30000

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_msg_timer;
static GFont s_font20;
static GFont s_font40;

static char s_time_buf[6];
static char s_date_buf[8];
static int s_battery = 100;
static bool s_charging = false;
static int s_line2_index = 0;
static int s_line1_index = 0;

// 2行目を先に選び、その2行目に合う1行目候補から1つ選んで文にする
// 1行7文字まで(内寸126px)
typedef struct {
  const char *line2;
  const char *const *line1s;
  int line1_count;
} Message;

#define L1(...) (const char *const[]){__VA_ARGS__},   (int)(sizeof((const char *const[]){__VA_ARGS__}) / sizeof(const char *))

static const Message MESSAGES[] = {
  {"あらわれた!",   L1("スライムが", "ドラゴンが", "ミミックが", "まおうが", "まもののむれが", "ゴーストが")},
  {"めざめた...",   L1("まおうが", "ドラゴンが", "ミミックが", "まけんが")},
  {"にげだした!",   L1("おれは", "スライムは", "まものたちは", "おうさまは")},
  {"レベルアップ!", L1("おれは", "スライムは", "なかまは")},
  {"ひとやすみ。",  L1("おれは", "まおうは", "ドラゴンは", "スライムは")},
  {"ねむっている",  L1("おれは", "まおうは", "ドラゴンは", "おうさまは")},
  {"みがまえた!",   L1("おれは", "スライムは", "ドラゴンは")},
  {"こうげき!",     L1("おれの", "スライムの", "まおうの", "ドラゴンの")},
  {"いちげき!",     L1("つうこんの", "かいしんの")},
  {"かいふくした!", L1("HPが", "MPが", "おれは")},
  {"たりない!",     L1("MPが", "ゴールドが", "やくそうが", "コーヒーが")},
  {"みつけた!",     L1("たからばこを", "やくそうを", "ゴールドを", "ひみつのみちを")},
  {"つかった。",    L1("やくそうを", "まほうを", "MPを")},
  {"のんだ!",       L1("ポーションを", "くすりを", "どくを")},
  {"おこらなかった", L1("しかし なにも")},
  {"おたのしみ?",   L1("ゆうべは")},
  {"きろくした。",  L1("ぼうけんを", "セーブデータを")},
  {"100Gえた!",     L1("おれは", "スライムは")},
  {"にげろ!",       L1("ミミックだ!", "ドラゴンだ!", "まおうだ!")},
  {"おなかすいた",   L1("おれは", "スライムは", "まものたちは", "おうさまは")},
  {"ころんだ",       L1("おれは", "スライムは", "まものたちは", "おうさまは")},
  {"しっぱい!",      L1("つうこんの", "かいしんの")},
  {"しんかした!",    L1("だいまおうは", "まおうは")},
  {"ほんきをだした", L1("まおうが", "だいまおうが", "ドラゴンが")},
  {"へんしんした!",  L1("まおうが", "だいまおうが", "ミミックが")},
  {"ほのおをはいた", L1("ドラゴンが", "だいまおうが", "まおうが")},
  {"なかまをよんだ", L1("まおうは", "だいまおうは", "ミミックは", "スライムは")},
  {"にげられない!",  L1("しかし", "ボスからは")},
  {"さまよってる",   L1("おれたちは")},
  {"ないている",     L1("おれは", "まおうは", "スライムは", "ミミックは")},
  {"とけた...",      L1("スライムが", "アイスが", "おれが")},
  {"さびしそうだ",   L1("まおうは", "ミミックは", "ドラゴンは")},
  {"おどりだした!",  L1("おれは", "スライムは", "まものたちは", "おうさまは")},
  {"うたいだした!",  L1("おれは", "スライムは", "まおうは")},
  {"なかまになった", L1("スライムが", "まおうが", "ミミックが", "おうさまが")},
  {"まいごになった", L1("おれは", "ゆうしゃは", "おうさまは")},
  {"ひっさつわざ!",  L1("おれの", "ゆうしゃの", "だいまおうの")},
  {"けんをぬいた",   L1("おれは", "ゆうしゃは", "まおうは")},
  {"たちあがった!",  L1("おれは", "ゆうしゃは", "なかまは", "ドラゴンは")},
  {"かくせいした!",  L1("おれは", "ゆうしゃは", "ドラゴンは", "まおうは")},
  {"まもりぬいた!",  L1("おれは", "なかまを", "このまちを")},
  {"よあけがきた",   L1("そして", "たたかいのあと")},
  {"でんせつとなる", L1("おれは", "ゆうしゃは", "このたびは")},
  {"たおれない!",    L1("おれは", "ゆうしゃは", "なかまは")},
  {"かぜがふいた",   L1("そのとき", "しずかに")},
  {"ひかりをえた!",  L1("おれは", "ゆうしゃは", "せいけんは")},
};
#define MSG_COUNT ((int)(sizeof(MESSAGES) / sizeof(MESSAGES[0])))

static const char *const LOW_BATTERY_LINE1 = "HPが";
static const char *const LOW_BATTERY_LINE2 = "すくない!";

static void pick_message(void) {
  int next = rand() % MSG_COUNT;
  if (MSG_COUNT > 1 && next == s_line2_index) {
    next = (next + 1) % MSG_COUNT;
  }
  s_line2_index = next;
  s_line1_index = rand() % MESSAGES[next].line1_count;
}

static void current_message(const char **line1, const char **line2) {
  if (s_battery <= 20 && !s_charging && (s_line2_index % 2 == 0)) {
    *line1 = LOW_BATTERY_LINE1;
    *line2 = LOW_BATTERY_LINE2;
    return;
  }
  const Message *m = &MESSAGES[s_line2_index];
  *line1 = m->line1s[s_line1_index];
  *line2 = m->line2;
}

// ドラクエ風の窓: 黒い太枠に白い中身
static void draw_window(GContext *ctx, GRect r) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, r, 5, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, grect_inset(r, GEdgeInsets(2)), 4, GCornersAll);
}

static void draw_line(GContext *ctx, const char *text, GFont font, int x, int y, int w) {
  graphics_draw_text(ctx, text, font, GRect(x, y, w, 30),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

// HH:MMを1文字ずつ詰めて描く(そのままの送り36pxだと180pxで収まらない)
static void draw_big_time(GContext *ctx, int y) {
  char ch[2] = {0, 0};
  int x = 0;
  for (int i = 0; i < 5; i++) {
    ch[0] = s_time_buf[i];
    if (ch[0] == ':') {
      draw_line(ctx, ch, s_font40, x - BIG_COLON_INK_X, y, 40);
      x += BIG_COLON_INK_W + BIG_GAP;
    } else {
      draw_line(ctx, ch, s_font40, x, y, 40);
      x += BIG_DIGIT_W + BIG_GAP;
    }
  }
}

static void draw_hp_bar(GContext *ctx, int x, int y, int w) {
  const int h = 12;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(x, y, w, h));
  int fill = (w - 4) * s_battery / 100;
  if (fill > 0) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(x + 2, y + 2, fill, h - 4), 0, GCornerNone);
  }
}

// 次へ送る▼
static void draw_cursor(GContext *ctx, int cx, int y) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (int i = 0; i < 5; i++) {
    graphics_fill_rect(ctx, GRect(cx - 5 + i, y + i, 10 - i * 2, 1), 0, GCornerNone);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  const int inner_w = b.size.w - TEXT_PAD_X * 2;

  // ステータス窓: 日付 / HPゲージ(電池)
  GRect status = GRect(0, 0, b.size.w, 54);
  draw_window(ctx, status);
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_line(ctx, s_date_buf, s_font20, TEXT_PAD_X, 6, inner_w);
  draw_line(ctx, "HP", s_font20, TEXT_PAD_X, 29, CHAR_ADV * 2);
  int bar_x = TEXT_PAD_X + CHAR_ADV * 2 + 4;
  draw_hp_bar(ctx, bar_x, 33, b.size.w - TEXT_PAD_X - bar_x);

  // 時刻
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_big_time(ctx, 57);

  // メッセージ窓
  GRect msg = GRect(0, 100, b.size.w, b.size.h - 100);
  draw_window(ctx, msg);
  const char *line1, *line2;
  current_message(&line1, &line2);
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_line(ctx, line1, s_font20, TEXT_PAD_X, 108, inner_w);
  draw_line(ctx, line2, s_font20, TEXT_PAD_X, 130, inner_w);
  draw_cursor(ctx, b.size.w / 2, b.size.h - 13);
}

static void update_time(struct tm *t) {
  strftime(s_time_buf, sizeof(s_time_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %d", t);
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 'a' - 'A';
  }
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  update_time(t);
  layer_mark_dirty(s_canvas);
}

static void battery_handler(BatteryChargeState state) {
  s_battery = state.charge_percent;
  s_charging = state.is_charging;
  if (s_canvas) layer_mark_dirty(s_canvas);
}

// メッセージは30秒ごとに切り替え
static void msg_timer_callback(void *data) {
  pick_message();
  if (s_canvas) layer_mark_dirty(s_canvas);
  s_msg_timer = app_timer_register(MSG_INTERVAL_MS, msg_timer_callback, NULL);
}

// 手首を振ると即切り替え、次の切り替えまでの30秒もリセット
static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_msg_timer) app_timer_cancel(s_msg_timer);
  msg_timer_callback(NULL);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_font20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PEBBLE_DOT_20));
  s_font40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PEBBLE_DOT_40));
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
  fonts_unload_custom_font(s_font20);
  fonts_unload_custom_font(s_font40);
}

static void init(void) {
  srand(time(NULL));
  pick_message();

  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  time_t now = time(NULL);
  update_time(localtime(&now));
  battery_handler(battery_state_service_peek());

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  accel_tap_service_subscribe(tap_handler);
  s_msg_timer = app_timer_register(MSG_INTERVAL_MS, msg_timer_callback, NULL);
}

static void deinit(void) {
  if (s_msg_timer) app_timer_cancel(s_msg_timer);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
