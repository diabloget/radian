#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

// Colors
const Fl_Color C_BG = fl_rgb_color(30, 30, 35);
const Fl_Color C_PANEL = fl_rgb_color(45, 45, 50);
const Fl_Color C_ACCENT = fl_rgb_color(0, 122, 204);
const Fl_Color C_TEXT = fl_rgb_color(230, 230, 230);
const Fl_Color C_BTN = fl_rgb_color(60, 60, 65);
const Fl_Color C_RED = fl_rgb_color(220, 50, 50);

struct AppConfig {
  int test = KEY_8, plus = KEY_0, minus = KEY_9, f_plus = KEY_EQUAL,
      f_minus = KEY_MINUS, cancel = KEY_BACKSPACE;
} cfg;

struct Profile {
  string name;
  int counts;
};

atomic<int> raw_counts(10000), capturing_mode(-1);
atomic<bool> movement_active(false);
vector<Profile> profiles;

Fl_Group *main_grp, *settings_grp;
Fl_Value_Input *val_input;
Fl_Input *inp_profile_name;
Fl_Choice *choice_profiles;
vector<Fl_Button *> bind_btns;

// Dynamic Path Logic
string get_config_path() {
  const char *appimage_env = getenv("APPIMAGE");
  if (appimage_env) {
    string full_path(appimage_env);
    size_t last_slash = full_path.find_last_of('/');
    if (last_slash != string::npos) {
      return full_path.substr(0, last_slash + 1) + "radian.cfg";
    }
  }
  return "radian.cfg";
}

// IO Utils
void save_data() {
  string path = get_config_path();
  ofstream f(path);
  if (!f.is_open())
    return;
  f << cfg.test << " " << cfg.plus << " " << cfg.minus << " " << cfg.f_plus
    << " " << cfg.f_minus << " " << cfg.cancel << endl;
  for (const auto &p : profiles)
    f << p.name << "," << p.counts << endl;

  f.close();

  // Makes sure the file is writable by the user
  const char *sudo_uid = getenv("SUDO_UID");
  const char *sudo_gid = getenv("SUDO_GID");
  const char *pkexec_uid = getenv("PKEXEC_UID");

  uid_t user_uid = -1;
  gid_t user_gid = -1;

  if (sudo_uid && sudo_gid) {
    user_uid = atoi(sudo_uid);
    user_gid = atoi(sudo_gid);
  } else if (pkexec_uid) {
    user_uid = atoi(pkexec_uid);
    user_gid = user_uid;
  }

  if (user_uid != (uid_t)-1) {
    if (chown(path.c_str(), user_uid, user_gid) == -1) {
      perror("chown warning");
    }
  }
  chmod(path.c_str(), 0644);
}

void load_data() {
  string path = get_config_path();
  ifstream f(path);
  if (!f.is_open()) {
    profiles.push_back({"Default", 10000});
    save_data();
    return;
  }
  string line;
  if (getline(f, line)) {
    stringstream ss(line);
    ss >> cfg.test >> cfg.plus >> cfg.minus >> cfg.f_plus >> cfg.f_minus;
    if (!(ss >> cfg.cancel)) {
      cfg.cancel = KEY_BACKSPACE;
    }
  }
  profiles.clear();
  while (getline(f, line)) {
    if (line.empty())
      continue;
    stringstream ss(line);
    string n, c;
    if (getline(ss, n, ',') && getline(ss, c))
      profiles.push_back({n, stoi(c)});
  }
  if (profiles.empty())
    profiles.push_back({"Default", 10000});
}

void refresh_profile_menu() {
  choice_profiles->clear();
  for (const auto &p : profiles)
    choice_profiles->add(p.name.c_str());
  for (int i = 0; i < choice_profiles->size(); i++)
    ((Fl_Menu_Item *)choice_profiles->menu())[i].labelcolor(C_TEXT);

  if (choice_profiles->size() > 0) {
    choice_profiles->value(0);
  }

  choice_profiles->redraw();
}

// Virtual Mouse for the Kernel
class VirtualMouse {
  int fd;

public:
  VirtualMouse() {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0)
      return;
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    struct uinput_user_dev uidev = {};
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Radian-Input");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;
    (void)!write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);
    sleep(1);
  }

  // Allows for cancelable mouse movement
  void move_async(int total) {
    if (fd < 0)
      return;

    movement_active = true;
    int moved = 0, chunk = 10;
    struct input_event ev = {};

    while (moved < total && movement_active) {
      int step = (total - moved > chunk) ? chunk : (total - moved);
      ev.type = EV_REL;
      ev.code = REL_X;
      ev.value = step;
      (void)!write(fd, &ev, sizeof(ev));
      ev.type = EV_SYN;
      ev.code = SYN_REPORT;
      ev.value = 0;
      (void)!write(fd, &ev, sizeof(ev));
      moved += step;
      this_thread::sleep_for(chrono::milliseconds(1));
    }

    movement_active = false;
  }

  // Thread needed for both mouse movement and keyboard listening
  void move(int total) {
    if (movement_active) {
      movement_active = false;
      this_thread::sleep_for(chrono::milliseconds(50));
    }

    thread([this, total]() { this->move_async(total); }).detach();
  }
};

void keyboard_loop(VirtualMouse *mouse) {
  struct libevdev *dev = NULL;
  int fd = -1;
  for (int i = 0; i < 32; i++) {
    string p = "/dev/input/event" + to_string(i);
    fd = open(p.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd >= 0 && libevdev_new_from_fd(fd, &dev) >= 0 &&
        libevdev_has_event_code(dev, EV_KEY, KEY_ENTER))
      break;
    if (fd >= 0) {
      close(fd);
      fd = -1;
    }
  }
  if (fd < 0)
    return;

  while (true) {
    struct input_event ev;
    if (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0 &&
        ev.type == EV_KEY && ev.value == 1) {
      int cap = capturing_mode.load();

      if (cap != -1) {
        // Keybind capture
        int *targets[] = {&cfg.test,   &cfg.plus,    &cfg.minus,
                          &cfg.f_plus, &cfg.f_minus, &cfg.cancel};
        *targets[cap] = ev.code;
        capturing_mode = -1;
        Fl::lock();
        bind_btns[cap]->label(libevdev_event_code_get_name(EV_KEY, ev.code));
        Fl::unlock();
        Fl::awake();
      } else {
        // This section is to verify if the movement was canceled
        if (ev.code == cfg.cancel && movement_active) {
          movement_active = false;
        } else if (ev.code == cfg.test)
          mouse->move(raw_counts);
        else if (ev.code == cfg.plus)
          raw_counts += 50;
        else if (ev.code == cfg.minus)
          raw_counts -= 50;
        else if (ev.code == cfg.f_plus)
          raw_counts += 1;
        else if (ev.code == cfg.f_minus)
          raw_counts -= 1;

        if (raw_counts < 0)
          raw_counts = 0;

        Fl::lock();
        val_input->value(raw_counts);
        Fl::unlock();
        Fl::awake();
      }
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

// UI Helpers
void style_btn(Fl_Button *b, Fl_Color c = C_BTN, Fl_Color t = C_TEXT) {
  b->box(FL_FLAT_BOX);
  b->color(c);
  b->labelcolor(t);
  b->clear_visible_focus();
}

void switch_view_cb(Fl_Widget *, void *v) {
  if ((long)v) {
    main_grp->hide();
    settings_grp->show();
  } else {
    settings_grp->hide();
    main_grp->show();
    save_data();
  }
}

void reset_defaults_cb(Fl_Widget *, void *) {
  cfg.test = KEY_8;
  cfg.plus = KEY_0;
  cfg.minus = KEY_9;
  cfg.f_plus = KEY_EQUAL;
  cfg.f_minus = KEY_MINUS;
  cfg.cancel = KEY_BACKSPACE;

  // We rely on the creation order of bind_btns
  if (bind_btns.size() >= 6) {
    bind_btns[0]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.test));
    bind_btns[1]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.plus));
    bind_btns[2]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.minus));
    bind_btns[3]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.f_plus));
    bind_btns[4]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.f_minus));
    bind_btns[5]->copy_label(libevdev_event_code_get_name(EV_KEY, cfg.cancel));
  }
}

void link_cb(Fl_Widget *w, void *v) {
  const char *url = ((long)v) ? "https://github.com/diabloget/radian"
                              : "https://github.com/diabloget";
  Fl::copy(url, strlen(url), 1);
  const char *old = w->label();
  w->label("COPIED!");
  w->redraw();
  thread([w, old]() {
    this_thread::sleep_for(chrono::seconds(2));
    Fl::lock();
    w->label(old);
    w->redraw();
    Fl::unlock();
    Fl::awake();
  }).detach();
}

void save_profile_cb(Fl_Widget *, void *) {
  string name = inp_profile_name->value();
  if (name.empty())
    return;

  int current_val = (int)val_input->value();
  raw_counts.store(current_val);

  bool found = false;
  for (auto &p : profiles) {
    if (p.name == name) {
      p.counts = current_val;
      found = true;
      break;
    }
  }
  if (!found)
    profiles.push_back({name, current_val});

  save_data();
  refresh_profile_menu();

  for (size_t i = 0; i < profiles.size(); ++i) {
    if (profiles[i].name == name) {
      choice_profiles->value((int)i);
      break;
    }
  }
}

void make_bind_row(int &y, const char *lbl, int key_code, int cap_idx) {
  auto *l = new Fl_Box(60, y, 150, 40, lbl);
  l->labelcolor(C_TEXT);
  l->labelsize(16);
  l->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  auto *b = new Fl_Button(240, y, 330, 40);
  b->copy_label(libevdev_event_code_get_name(EV_KEY, key_code));
  b->labelsize(14);
  style_btn(b);
  b->callback(
      [](Fl_Widget *w, void *v) {
        w->label("WAITING...");
        capturing_mode = (int)(long)v;
      },
      (void *)(long)cap_idx);
  bind_btns.push_back(b);
  y += 55;
}

int main(int argc, char **argv) {
  load_data();
  Fl::lock();
  Fl::scheme("gtk+");
  Fl_Tooltip::delay(0.1f);
  Fl_Tooltip::color(C_PANEL);
  Fl_Tooltip::textcolor(C_TEXT);

  VirtualMouse vMouse;
  thread(keyboard_loop, &vMouse).detach();

  Fl_Window *win = new Fl_Window(630, 480, "Radian");
  win->color(C_BG);

  main_grp = new Fl_Group(0, 0, 630, 480);
  auto *t = new Fl_Box(0, 30, 630, 45, "Match your sens");
  t->labelcolor(C_ACCENT);
  t->labelfont(FL_HELVETICA_BOLD);
  t->labelsize(33);

  auto *st = new Fl_Box(357, 67, 150, 30, "with ");
  st->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  st->labelsize(16);
  st->labelcolor(C_TEXT);

  auto *br = new Fl_Button(394, 67, 150, 30, "Radian");
  style_btn(br, C_BG, C_RED);
  br->box(FL_NO_BOX);
  br->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  br->labelfont(FL_HELVETICA_BOLD);
  br->labelsize(16);
  br->callback(link_cb, (void *)1);

  auto *mb = new Fl_Button(22, 25, 82, 37, "MENU");
  style_btn(mb);
  mb->labelsize(16);
  mb->callback(switch_view_cb, (void *)1);

  auto *h = new Fl_Button(562, 22, 45, 45, "?");
  style_btn(h, C_ACCENT, FL_WHITE);
  h->labelsize(18);
  h->tooltip("CONTROLS:\n[8] Do 360\n[0] Increase (+50)\n[9] Decrease "
             "(-50)\n[-/+] Fine Tune (1)\n[BKSP] Cancel Movement");

  choice_profiles = new Fl_Choice(90, 130, 450, 45);
  choice_profiles->color(C_BTN);
  choice_profiles->textcolor(C_TEXT);
  choice_profiles->textsize(20);
  choice_profiles->clear_visible_focus();
  choice_profiles->callback([](Fl_Widget *, void *) {
    int idx = choice_profiles->value();
    if (idx >= 0 && (size_t)idx < profiles.size()) {
      raw_counts = profiles[idx].counts;
      val_input->value(raw_counts);
      inp_profile_name->value(profiles[idx].name.c_str());
    }
  });
  refresh_profile_menu();

  val_input = new Fl_Value_Input(90, 205, 450, 45, "Raw Counts");
  val_input->color(C_BTN);
  val_input->textcolor(C_ACCENT);
  val_input->labelcolor(C_TEXT);
  val_input->textsize(21);
  val_input->labelsize(18);
  val_input->align(FL_ALIGN_TOP_LEFT);
  val_input->value(raw_counts);
  val_input->clear_visible_focus();
  val_input->callback([](Fl_Widget *w, void *) {
    raw_counts = (int)((Fl_Value_Input *)w)->value();
  });

  inp_profile_name = new Fl_Input(90, 290, 450, 45, "Profile Name");
  inp_profile_name->color(C_BTN);
  inp_profile_name->textcolor(C_TEXT);
  inp_profile_name->labelcolor(C_TEXT);
  inp_profile_name->textsize(20);
  inp_profile_name->labelsize(18);
  inp_profile_name->align(FL_ALIGN_TOP_LEFT);
  if (!profiles.empty())
    inp_profile_name->value(profiles[0].name.c_str());

  auto *btn_save = new Fl_Button(90, 370, 450, 45, "SAVE PROFILE");
  style_btn(btn_save, C_ACCENT, FL_WHITE);
  btn_save->labelfont(FL_HELVETICA_BOLD);
  btn_save->labelsize(16);
  btn_save->callback(save_profile_cb);

  main_grp->end();

  settings_grp = new Fl_Group(0, 0, 630, 480);
  settings_grp->hide();

  auto *b_back = new Fl_Button(22, 25, 82, 37, "BACK");
  style_btn(b_back);
  b_back->labelsize(16);
  b_back->callback(switch_view_cb, (void *)0);

  auto *b_reset = new Fl_Button(562, 25, 45, 37, "DEF");
  style_btn(b_reset, C_ACCENT, FL_WHITE);
  b_reset->labelsize(16);
  b_reset->tooltip("Set Default Values");
  b_reset->callback(reset_defaults_cb);

  auto *t_set = new Fl_Box(0, 30, 630, 45, "Keybinds");
  t_set->labelcolor(C_TEXT);
  t_set->labelfont(FL_HELVETICA_BOLD);
  t_set->labelsize(28);

  int y_pos = 100;
  make_bind_row(y_pos, "Test 360", cfg.test, 0);
  make_bind_row(y_pos, "Add +50", cfg.plus, 1);
  make_bind_row(y_pos, "Sub -50", cfg.minus, 2);
  make_bind_row(y_pos, "Fine Tune (+)", cfg.f_plus, 3);
  make_bind_row(y_pos, "Fine Tune (-)", cfg.f_minus, 4);
  make_bind_row(y_pos, "Cancel Move", cfg.cancel, 5);

  auto *footer = new Fl_Button(0, 445, 630, 30, "Created by Diabloget");
  style_btn(footer, C_BG, C_ACCENT);
  footer->box(FL_NO_BOX);
  footer->labelsize(14);
  footer->callback(link_cb, (void *)0);

  settings_grp->end();

  win->end();
  win->show(argc, argv);
  return Fl::run();
}