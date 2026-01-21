#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <pwd.h>
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

// UI Helpers
void style_btn(Fl_Button *b, Fl_Color c = C_BTN, Fl_Color t = C_TEXT) {
  b->box(FL_FLAT_BOX);
  b->color(c);
  b->labelcolor(t);
  b->clear_visible_focus();
}

// Custom Styled Dialog
int show_custom_dialog(const char *title, const char *msg,
                       const char *btn_ok_txt,
                       const char *btn_cancel_txt = nullptr) {
  Fl_Window *win = new Fl_Window(440, 210, "Radian Alert");
  win->color(C_BG);
  win->set_modal();

  // Header section
  Fl_Group *header_grp = new Fl_Group(20, 20, 400, 40);

  Fl_Box *icon = new Fl_Box(20, 20, 30, 30, "ℹ");
  icon->box(FL_NO_BOX);
  icon->labelcolor(C_ACCENT);
  icon->labelsize(34);
  icon->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

  Fl_Box *header = new Fl_Box(60, 20, 340, 30, title);
  header->box(FL_NO_BOX);
  header->labelcolor(C_TEXT);
  header->labelfont(FL_HELVETICA_BOLD);
  header->labelsize(19);
  header->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

  header_grp->end();

  // Message body
  Fl_Box *message = new Fl_Box(25, 70, 390, 80, msg);
  message->box(FL_NO_BOX);
  message->labelcolor(C_TEXT);
  message->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
  message->labelsize(15);

  // Buttons
  int result = 0;
  int btn_y = 160;
  int btn_w = 130;
  int btn_h = 35;

  if (btn_cancel_txt) {
    Fl_Button *btn_cancel =
        new Fl_Button(30, btn_y, btn_w, btn_h, btn_cancel_txt);
    style_btn(btn_cancel, C_BTN, C_TEXT);
    btn_cancel->callback(
        [](Fl_Widget *w, void *v) {
          *(int *)v = 0;
          w->window()->hide();
        },
        &result);
  }

  int ok_x = btn_cancel_txt ? (440 - 30 - btn_w) : (440 / 2 - btn_w / 2);
  Fl_Button *btn_ok = new Fl_Button(ok_x, btn_y, btn_w, btn_h, btn_ok_txt);
  style_btn(btn_ok, C_ACCENT, FL_WHITE);
  btn_ok->callback(
      [](Fl_Widget *w, void *v) {
        *(int *)v = 1;
        w->window()->hide();
      },
      &result);

  win->show();
  while (win->shown())
    Fl::wait();
  delete win;
  return result;
}

// Dynamic Path Logic
string get_config_path() {
  const char *appimage_path = getenv("APPIMAGE");
  if (appimage_path) {
    string full_path(appimage_path);
    size_t last_slash = full_path.find_last_of('/');
    if (last_slash != string::npos) {
      string dir = full_path.substr(0, last_slash + 1);
      string config_path = dir + "radian.cfg";

      FILE *fp = fopen(config_path.c_str(), "a+");
      if (fp) {
        fclose(fp);
        return config_path;
      }
    }
  }
  return "./radian.cfg";
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
    if (!(ss >> cfg.cancel))
      cfg.cancel = KEY_BACKSPACE;
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

// System Integration
bool check_uinput_access() {
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd >= 0) {
    close(fd);
    return true;
  }
  return false;
}

bool is_in_input_group() {
  int ngroups = 0;
  getgrouplist(getenv("USER") ?: "nobody", getgid(), NULL, &ngroups);
  vector<gid_t> groups(ngroups);
  getgrouplist(getenv("USER") ?: "nobody", getgid(), groups.data(), &ngroups);

  struct group *input_grp = getgrnam("input");
  if (!input_grp)
    return false;

  for (int i = 0; i < ngroups; i++) {
    if (groups[i] == input_grp->gr_gid)
      return true;
  }
  return false;
}

bool setup_permissions() {
  const char *user = getenv("USER");
  if (!user) {
    struct passwd *pw = getpwuid(getuid());
    user = pw ? pw->pw_name : "nobody";
  }

  // Explicitly load module, add group, and FORCE permissions
  stringstream cmd;
  cmd << "pkexec sh -c '";
  cmd << "modprobe uinput; ";
  cmd << "groupadd -f input; ";
  cmd << "usermod -a -G input " << user << "; ";
  cmd << "echo \"KERNEL==\\\"uinput\\\", MODE=\\\"0660\\\", "
         "GROUP=\\\"input\\\", OPTIONS+=\\\"static_node=uinput\\\"\" > "
         "/etc/udev/rules.d/99-radian-input.rules; ";
  cmd << "udevadm control --reload-rules; ";
  cmd << "udevadm trigger; ";
  // Permissions are applied manually to avoid failure
  cmd << "chown root:input /dev/uinput; ";
  cmd << "chmod 0660 /dev/uinput";
  cmd << "'";

  return system(cmd.str().c_str()) == 0;
}

// Virtual Mouse
class VirtualMouse {
  int fd;

public:
  VirtualMouse() : fd(-1) {
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

    if (write(fd, &uidev, sizeof(uidev)) < 0) {
      close(fd);
      fd = -1;
      return;
    }
    ioctl(fd, UI_DEV_CREATE);
    sleep(1);
  }

  ~VirtualMouse() {
    if (fd >= 0) {
      ioctl(fd, UI_DEV_DESTROY);
      close(fd);
    }
  }

  bool is_valid() const { return fd >= 0; }

  void move_async(int total) {
    if (fd < 0 || total <= 0)
      return;

    movement_active = true;
    int moved = 0, chunk = 10;
    struct input_event ev = {};

    while (moved < total && movement_active) {
      int step = min(chunk, total - moved);
      ev.type = EV_REL;
      ev.code = REL_X;
      ev.value = step;
      write(fd, &ev, sizeof(ev));
      ev.type = EV_SYN;
      ev.code = SYN_REPORT;
      ev.value = 0;
      write(fd, &ev, sizeof(ev));

      moved += step;
      this_thread::sleep_for(chrono::milliseconds(1));
    }
    movement_active = false;
  }

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
    if (fd >= 0 && libevdev_new_from_fd(fd, &dev) >= 0) {
      if (libevdev_has_event_code(dev, EV_KEY, KEY_ENTER) &&
          libevdev_has_event_code(dev, EV_KEY, KEY_A) &&
          !libevdev_has_event_code(dev, EV_REL, REL_X)) {
        break;
      }
      libevdev_free(dev);
      close(fd);
      fd = -1;
      dev = NULL;
    } else if (fd >= 0) {
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
        int *targets[] = {&cfg.test,   &cfg.plus,    &cfg.minus,
                          &cfg.f_plus, &cfg.f_minus, &cfg.cancel};
        if (cap >= 0 && cap < 6) {
          *targets[cap] = ev.code;
          capturing_mode = -1;

          Fl::lock();
          bind_btns[cap]->label(libevdev_event_code_get_name(EV_KEY, ev.code));
          Fl::unlock();
          Fl::awake();
        }
      } else {
        if (ev.code == cfg.cancel && movement_active) {
          movement_active = false;
        } else if (ev.code == cfg.test) {
          mouse->move(raw_counts);
        } else if (ev.code == cfg.plus) {
          raw_counts += 50;
        } else if (ev.code == cfg.minus) {
          raw_counts -= 50;
        } else if (ev.code == cfg.f_plus) {
          raw_counts += 1;
        } else if (ev.code == cfg.f_minus) {
          raw_counts -= 1;
        }

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

  if (dev)
    libevdev_free(dev);
  if (fd >= 0)
    close(fd);
}

void refresh_profile_menu() {
  choice_profiles->clear();
  for (const auto &p : profiles)
    choice_profiles->add(p.name.c_str());

  for (int i = 0; i < choice_profiles->size(); i++)
    ((Fl_Menu_Item *)choice_profiles->menu())[i].labelcolor(C_TEXT);

  if (choice_profiles->size() > 0)
    choice_profiles->value(0);
  choice_profiles->redraw();
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
  auto *l = new Fl_Box(30, y, 120, 30, lbl);
  l->labelcolor(C_TEXT);
  l->labelsize(14);
  l->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

  auto *b = new Fl_Button(160, y, 220, 30);
  b->copy_label(libevdev_event_code_get_name(EV_KEY, key_code));
  b->labelsize(13);
  style_btn(b);

  b->callback(
      [](Fl_Widget *w, void *v) {
        w->label("WAITING...");
        capturing_mode = (int)(long)v;
      },
      (void *)(long)cap_idx);

  bind_btns.push_back(b);
  y += 38;
}

int main(int argc, char **argv) {
  Fl::scheme("gtk+");

  if (!check_uinput_access()) {
    int choice = show_custom_dialog(
        "Permissions Required",
        "Radian needs direct access to input devices.\n"
        "This requires a ONE-TIME setup with your password.\n\n"
        "After this, it will not ask for root again.",
        "Setup", "Not Now");

    if (choice == 1) {
      if (setup_permissions()) {
        if (check_uinput_access()) {
          if (!is_in_input_group()) {
            show_custom_dialog(
                "Setup Complete",
                "Permissions applied.\n"
                "Please LOG OUT and LOG IN once to finalize access.",
                "OK");
            return 0;
          } else {
            show_custom_dialog("Success",
                               "Permissions active.\nRestarting application...",
                               "OK");
            execvp(argv[0], argv);
          }
        } else {
          show_custom_dialog("Setup Complete",
                             "Please LOG OUT and LOG IN to apply changes.\n"
                             "Radian will work normally afterwards.",
                             "OK");
          return 0;
        }
      } else {
        show_custom_dialog("Setup Failed",
                           "Could not apply permissions.\n"
                           "Check your password or system config.",
                           "Close");
        return 1;
      }
    } else {
      return 0;
    }
  }

  load_data();

  Fl::lock();
  Fl_Tooltip::delay(0.1f);
  Fl_Tooltip::color(C_PANEL);
  Fl_Tooltip::textcolor(C_TEXT);

  VirtualMouse vMouse;
  if (!vMouse.is_valid()) {
    show_custom_dialog("Error",
                       "Failed to create virtual mouse.\n"
                       "Check udev rules.",
                       "Close");
    return 1;
  }

  thread(keyboard_loop, &vMouse).detach();

  Fl_Window *win = new Fl_Window(420, 320, "Radian");
  win->color(C_BG);

  main_grp = new Fl_Group(0, 0, 420, 320);
  auto *t = new Fl_Box(0, 20, 420, 35, "Match your sens");
  t->labelcolor(C_ACCENT);
  t->labelfont(FL_HELVETICA_BOLD);
  t->labelsize(28);

  auto *st = new Fl_Box(240, 52, 100, 30, "with ");
  st->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  st->labelsize(15);
  st->labelcolor(C_TEXT);

  auto *br = new Fl_Button(275, 52, 100, 30, "Radian");
  style_btn(br, C_BG, C_RED);
  br->box(FL_NO_BOX);
  br->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  br->labelfont(FL_HELVETICA_BOLD);
  br->labelsize(15);
  br->callback(link_cb, (void *)1);

  auto *mb = new Fl_Button(15, 10, 60, 30, "MENU");
  style_btn(mb);
  mb->labelsize(12);
  mb->callback(switch_view_cb, (void *)1);

  auto *h = new Fl_Button(378, 11, 28, 28, "ℹ");
  h->box(FL_OVAL_BOX);
  h->color(C_ACCENT);
  h->labelcolor(FL_WHITE);
  h->labelsize(14);
  h->clear_visible_focus();
  h->tooltip("CONTROLS:\n[8] Do 360\n[0] Increase (+50)\n[9] Decrease "
             "(-50)\n[-/+] Fine Tune (1)\n[BKSP] Cancel Movement");

  choice_profiles = new Fl_Choice(60, 95, 300, 30);
  choice_profiles->color(C_BTN);
  choice_profiles->textcolor(C_TEXT);
  choice_profiles->textsize(15);
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

  val_input = new Fl_Value_Input(60, 142, 300, 30, "Raw Counts");
  val_input->color(C_BTN);
  val_input->textcolor(C_ACCENT);
  val_input->labelcolor(C_TEXT);
  val_input->textsize(16);
  val_input->labelsize(13);
  val_input->align(FL_ALIGN_TOP_LEFT);
  val_input->value(raw_counts);
  val_input->clear_visible_focus();
  val_input->callback([](Fl_Widget *w, void *) {
    raw_counts = (int)((Fl_Value_Input *)w)->value();
  });

  inp_profile_name = new Fl_Input(60, 195, 300, 30, "Profile Name");
  inp_profile_name->color(C_BTN);
  inp_profile_name->textcolor(C_TEXT);
  inp_profile_name->labelcolor(C_TEXT);
  inp_profile_name->textsize(15);
  inp_profile_name->labelsize(13);
  inp_profile_name->align(FL_ALIGN_TOP_LEFT);
  if (!profiles.empty())
    inp_profile_name->value(profiles[0].name.c_str());

  auto *btn_save = new Fl_Button(60, 248, 300, 35, "SAVE PROFILE");
  style_btn(btn_save, C_ACCENT, FL_WHITE);
  btn_save->labelfont(FL_HELVETICA_BOLD);
  btn_save->labelsize(14);
  btn_save->callback(save_profile_cb);

  main_grp->end();

  settings_grp = new Fl_Group(0, 0, 420, 320);
  settings_grp->hide();

  auto *b_back = new Fl_Button(15, 10, 60, 30, "BACK");
  style_btn(b_back);
  b_back->labelsize(12);
  b_back->callback(switch_view_cb, (void *)0);

  auto *b_reset = new Fl_Button(378, 11, 28, 28, "⟲");
  b_reset->box(FL_OVAL_BOX);
  b_reset->color(C_ACCENT);
  b_reset->labelcolor(FL_WHITE);
  b_reset->labelsize(16);
  b_reset->clear_visible_focus();
  b_reset->tooltip("Set Default Values");
  b_reset->callback(reset_defaults_cb);

  auto *t_set = new Fl_Box(0, 10, 420, 30, "Keybinds");
  t_set->labelcolor(C_TEXT);
  t_set->labelfont(FL_HELVETICA_BOLD);
  t_set->labelsize(22);

  int y_pos = 50;
  make_bind_row(y_pos, "Test 360", cfg.test, 0);
  make_bind_row(y_pos, "Add +50", cfg.plus, 1);
  make_bind_row(y_pos, "Sub -50", cfg.minus, 2);
  make_bind_row(y_pos, "Fine Tune (+)", cfg.f_plus, 3);
  make_bind_row(y_pos, "Fine Tune (-)", cfg.f_minus, 4);
  make_bind_row(y_pos, "Cancel Move", cfg.cancel, 5);

  auto *footer = new Fl_Button(0, 290, 420, 30, "Created by Diabloget");
  style_btn(footer, C_BG, C_ACCENT);
  footer->box(FL_NO_BOX);
  footer->labelsize(12);
  footer->callback(link_cb, (void *)0);

  settings_grp->end();

  win->end();
  win->show(argc, argv);
  return Fl::run();
}