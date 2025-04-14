# systemd-电源管理部分
## systemd电源管理流程分析

### systemctl reboot

```cpp
run() :src/systemctl/systemctl.c
-> systemctl_main(argc, argv); // case ACTION_SYSTEMCTL: 完成该调用。因为用户空间reboot是systemctl reboot的软连接
    -> static const Verb verbs[] = { { "reboot", VERB_ANY, 2,  VERB_ONLINE_ONLY, start_system_special },}
    -> const Verb *verb = verbs_find_verb(argv[optind], verbs);
    -> dispatch_verb(argc, argv, verbs, NULL);
        -> return verb->dispatch(left, argv, userdata); // 因为struct verb中int (* const dispatch)(int argc, char *argv[], void *userdata);赋值start_system_special
```
start_system_special 是一个静态函数，接受三个参数：argc（命令行参数个数）、argv（命令行参数数组）和 userdata（用户数据指针）。
注释：注释说明了该函数类似于 start_special 函数，但在用户模式下运行时会引发错误。其目的是确保某些操作只能在系统模式下执行，而不是在用户模式下。
该函数的主要作用是确保某些操作只能在系统模式下执行。如果在用户模式下尝试执行这些操作，函数会记录错误并返回错误码。否则，它会调用 start_special 函数继续执行。

```cpp
static int start_special(int argc, char *argv[], void *userdata)
-> a = verb_to_action(argv[0]);
->  if (a == ACTION_REBOOT) { r = update_reboot_parameter_and_warn(arg, false); }
-> r = logind_reboot(a);
-> r = start_unit(argc, argv, userdata);
```

```cpp
r = trivial_method(argc, argv, userdata);
-> r = bus_call_method(bus, bus_systemd_mgr, method, &error, NULL, NULL);
    -> r = sd_bus_call_methodv(bus, locator->destination, locator->path, locator->interface, member, error, reply, types, ap);
        -> return sd_bus_call(bus, m, 0, error, reply);
```

logind-dbus.c 处理Reboot/RebootWithFlags调用:
实际上也是转化为启动reboot.target,systemctl reboot和reboot这两个命令最后都会转换成systemctl reboot -f调用，也即是调用 systemd 的Reboot方法（bus_call_method(bus_systemd_mgr, "Reboot")），将它的管理器的objective（目标）设置为MANAGER_REBOOT（见dbus-manager.c:method_reboot()）。
```cpp
static int method_reboot(sd_bus_message *message, void *userdata, sd_bus_error *error)
->         return method_do_shutdown_or_sleep(
                        m, message,
                        SPECIAL_REBOOT_TARGET,
                        INHIBIT_SHUTDOWN,
                        "org.freedesktop.login1.reboot",
                        "org.freedesktop.login1.reboot-multiple-sessions",
                        "org.freedesktop.login1.reboot-ignore-inhibit",
                        NULL,
                        error);
    -> r = bus_manager_shutdown_or_sleep_now_or_later(m, unit_name, w, error);
        -> r = execute_shutdown_or_sleep(m, w, unit_name, error);
            -> bus_call_method(
                        m->bus,
                        bus_systemd_mgr,
                        "StartUnit",
                        error,
                        &reply,
                        "ss", unit_name, "replace-irreversibly");

```
然后systemd进行实际的reboot执行管理（reboot-force：服务配置文件）：

将emergency_action赋值给property_get_emergency_action，BUS_DEFINE_PROPERTY_GET_ENUM宏是一个属性获取函数，该函数返回一个枚举值。
当通过 D-Bus 接口请求 emergency_action 属性时，这个函数会被调用。
emergency_action: 这是属性的名称。D-Bus 客户端会使用这个名字来请求属性值。
EmergencyAction: 这是枚举类型的名称。该枚举类型定义了所有可能的紧急操作值。
```cpp
static BUS_DEFINE_PROPERTY_GET_ENUM(property_get_emergency_action, emergency_action, EmergencyAction);
-> void emergency_action(
                Manager *m,
                EmergencyAction action,
                EmergencyActionFlags options,
                const char *reboot_arg,
                int exit_status,
                const char *reason)
    -> (void) reboot(RB_AUTOBOOT);

// SuccessAction 对应emergency_action 中的reboot-force ，这是与服务配置文件systemd-reboot.service分析：
SD_BUS_PROPERTY("SuccessAction", "s", property_get_emergency_action, offsetof(Unit, success_action), SD_BUS_VTABLE_PROPERTY_CONST),

// 以上宏在const sd_bus_vtable bus_unit_vtable[] 数组中，该数组在src/core/dbus.c中使用，也就是作为target单元在bus中进行管理
static const BusObjectImplementation unit_object = {
        "/org/freedesktop/systemd1/unit",
        "org.freedesktop.systemd1.Unit",
        .fallback_vtables = BUS_FALLBACK_VTABLES(
                { bus_unit_vtable,        bus_unit_find }),
        .node_enumerator = bus_unit_enumerate,
};

// systemd中包含的主要类型
static const BusObjectImplementation bus_manager_object = {
        "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager",
        .vtables = BUS_VTABLES(bus_manager_vtable),
        .children = BUS_IMPLEMENTATIONS(
                        &job_object,
                        &unit_object,
                        &bus_automount_object,
                        &bus_device_object,
                        &bus_mount_object,
                        &bus_path_object,
                        &bus_scope_object,
                        &bus_service_object,
                        &bus_slice_object,
                        &bus_socket_object,
                        &bus_swap_object,
                        &bus_target_object,
                        &bus_timer_object),
};
```


systemd主函数逻辑：
```cpp
int main(int argc, char *argv[])  // src/core/main.c
// 以 PID=1 执行 /lib/systemd/systemd-shutdown
r = become_shutdown(shutdown_verb, retval);
    -> execve(SYSTEMD_SHUTDOWN_BINARY_PATH, (char **) command_line, env_block);

// shutdown_verb 分析
        (void) invoke_main_loop(m,
                                &saved_rlimit_nofile,
                                &saved_rlimit_memlock,
                                &reexecute,
                                &retval,
                                &shutdown_verb,
                                &fds,
                                &switch_root_dir,
                                &switch_root_init,
                                &error_message);
```


### systemctl suspend

```cpp
// src/path/path.c
static const char* const path_table[_SD_PATH_MAX] = {
        [SD_PATH_SYSTEMD_SLEEP]                   = "systemd-sleep",
        [SD_PATH_SYSTEMD_SHUTDOWN]                = "systemd-shutdown",
}

// src/systemctl/systemctl.c
static int systemctl_main(int argc, char *argv[]) {
        static const Verb verbs[] = {
                            { "suspend",               VERB_ANY, 1,        VERB_ONLINE_ONLY, start_system_special    },
        }
}

static int start_system_special(int argc, char *argv[], void *userdata) {
        /* Like start_special above, but raises an error when running in user mode */

        if (arg_scope != UNIT_FILE_SYSTEM)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Bad action for %s mode.",
                                       arg_scope == UNIT_FILE_GLOBAL ? "--global" : "--user");

        return start_special(argc, argv, userdata);
}

static int start_special(int argc, char *argv[], void *userdata)
-> a = verb_to_action(argv[0]);
-> r = logind_reboot(a);
-> r = start_unit(argc, argv, userdata);// 转换为suspend.target

static int start_unit(int argc, char *argv[], void *userdata)
-> method = verb_to_method(argv[0]);


static int method_suspend(sd_bus_message *message, void *userdata, sd_bus_error *error) {
        Manager *m = userdata;

        return method_do_shutdown_or_sleep(
                        m, message,
                        SPECIAL_SUSPEND_TARGET,
                        INHIBIT_SLEEP,
                        "org.freedesktop.login1.suspend",
                        "org.freedesktop.login1.suspend-multiple-sessions",
                        "org.freedesktop.login1.suspend-ignore-inhibit",
                        "suspend",
                        error);
}

        SD_BUS_METHOD_WITH_NAMES("Suspend",
                                 "b",
                                 SD_BUS_PARAM(interactive),
                                 NULL,,
                                 method_suspend,
                                 SD_BUS_VTABLE_UNPRIVILEGED),

// suspend主要流程：
run() //src/sleep/sleep.c
->return execute(modes, states);
 -> f = fopen("/sys/power/state", "we");

r = execute(sleep_config->suspend_modes, sleep_config->suspend_states);

// src/sleep/sleep.c
static int execute(char **modes, char **states)
-> r = write_state(&f, states);
static int write_state(FILE **f, char **states) {
        char **state;
        int r = 0;

        assert(f);
        assert(*f);

        STRV_FOREACH(state, states) {
                int k;

                k = write_string_stream(*f, *state, WRITE_STRING_FILE_DISABLE_BUFFER);
                if (k >= 0)
                        return 0;
                log_debug_errno(k, "Failed to write '%s' to /sys/power/state: %m", *state);
                if (r >= 0)
                        r = k;

                fclose(*f);
                *f = fopen("/sys/power/state", "we");
                if (!*f)
                        return -errno;
        }

        return r;
}
```

### systemctl如何命令转化为目标单元target

```cpp
// src/path/path.c
static const char* const path_table[_SD_PATH_MAX] = {
        [SD_PATH_SYSTEMD_SLEEP]                   = "systemd-sleep",
        [SD_PATH_SYSTEMD_SHUTDOWN]                = "systemd-shutdown",
}

// src/systemctl/systemctl.c
static int systemctl_main(int argc, char *argv[]) {
        static const Verb verbs[] = {
                            { "suspend",               VERB_ANY, 1,        VERB_ONLINE_ONLY, start_system_special    },
        }
}

static int start_system_special(int argc, char *argv[], void *userdata) {
        /* Like start_special above, but raises an error when running in user mode */

        if (arg_scope != UNIT_FILE_SYSTEM)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL),
                                       "Bad action for %s mode.",
                                       arg_scope == UNIT_FILE_GLOBAL ? "--global" : "--user");

        return start_special(argc, argv, userdata);
}

static int start_special(int argc, char *argv[], void *userdata)
-> a = verb_to_action(argv[0]);
-> r = logind_reboot(a);
-> r = start_unit(argc, argv, userdata);// 转换为suspend.target

```

### 如何转化未执行具体命令--powerofff
关注命令行数组，执行函数赋值。systemd根据你的命令，找到对应的分发函数，然后执行对应命令。
```cpp
// systemd主函数逻辑：
int main(int argc, char *argv[])  // src/core/main.c
// 以 PID=1 执行 /lib/systemd/systemd-shutdown
r = become_shutdown(shutdown_verb, retval);
    -> execve(SYSTEMD_SHUTDOWN_BINARY_PATH, (char **) command_line, env_block);

// shutdown_verb 分析
        (void) invoke_main_loop(m,
                                &saved_rlimit_nofile,
                                &saved_rlimit_memlock,
                                &reexecute,
                                &retval,
                                &shutdown_verb,
                                &fds,
                                &switch_root_dir,
                                &switch_root_init,
                                &error_message);

// 最终都是exec类似接口执行对应的二进制文件
```
