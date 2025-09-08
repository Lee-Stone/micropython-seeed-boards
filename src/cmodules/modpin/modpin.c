#include "py/runtime.h"
#include "py/obj.h"
#include "zephyr_device.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include "shared/runtime/mpirq.h"

typedef struct _pin_irq_obj_t {
    mp_irq_obj_t base;
    struct _pin_irq_obj_t *next;
    struct gpio_callback callback;
} pin_irq_obj_t;

typedef struct _pin_obj_t {
    mp_obj_base_t base;
    struct gpio_dt_spec spec;
    gpio_flags_t flags;
    pin_irq_obj_t *irq;
} pin_obj_t;

enum {
    PIN_MODE_IN = GPIO_INPUT,
    PIN_MODE_OUT = GPIO_OUTPUT,
    PIN_MODE_OPEN_DRAIN = GPIO_OUTPUT | GPIO_OPEN_DRAIN,
    PIN_MODE_PULL_UP = GPIO_PULL_UP,
    PIN_MODE_PULL_DOWN = GPIO_PULL_DOWN,
    PIN_MODE_IRQ_RISING = GPIO_INT_EDGE_RISING,
    PIN_MODE_IRQ_FALLING = GPIO_INT_EDGE_FALLING
};

static const mp_irq_methods_t pin_irq_methods;

static void gpio_callback_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    pin_irq_obj_t *irq = CONTAINER_OF(cb, pin_irq_obj_t, callback);
    mp_irq_handler(&irq->base);
}

static mp_obj_t pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 3, false); 

    pin_obj_t *self = m_new_obj(pin_obj_t);
    self->base.type = type;
    
    const char *pin_name = mp_obj_str_get_str(args[0]);
    self->flags = 0;
    self->irq = NULL;
    
    if (strcmp(pin_name, "D0") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d0), gpios);
    } else if (strcmp(pin_name, "D1") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d1), gpios);
    } else if (strcmp(pin_name, "D2") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d2), gpios);
    } else if (strcmp(pin_name, "D3") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d3), gpios);
    } else if (strcmp(pin_name, "D4") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d4), gpios);
    } else if (strcmp(pin_name, "D5") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d5), gpios);
    } else if (strcmp(pin_name, "D6") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d6), gpios);
    } else if (strcmp(pin_name, "D7") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d7), gpios);
    } else if (strcmp(pin_name, "D8") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d8), gpios);
    } else if (strcmp(pin_name, "D9") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d9), gpios);
    } else if (strcmp(pin_name, "D10") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(d10), gpios);
    }else if (strcmp(pin_name, "LED0") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    } else if (strcmp(pin_name, "SW0") == 0) {
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
    }else if (strcmp(pin_name, "EN") == 0){
        self->spec = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_ALIAS(en), gpios);
    }else {
        mp_raise_msg(&mp_type_ValueError, "Invalid pin name");
    }

    if (!gpio_is_ready_dt(&self->spec)) {
        mp_raise_msg(&mp_type_OSError, "GPIO device not ready");
    }

    if (n_args >= 2) {
        self->flags = mp_obj_get_int(args[1]);
    }

    if (n_args >= 3) {
        self->flags |= mp_obj_get_int(args[2]);
    }

    int ret = gpio_pin_configure_dt(&self->spec, self->flags);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pin_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);

    self->flags = parsed_args[ARG_mode].u_int;

    if (parsed_args[ARG_pull].u_obj != mp_const_none) {
        self->flags |= mp_obj_get_int(parsed_args[ARG_pull].u_obj);
    }

    uint init = 0;
    if (parsed_args[ARG_value].u_obj != MP_OBJ_NULL) {
        init = mp_obj_is_true(parsed_args[ARG_value].u_obj) ? GPIO_OUTPUT_INIT_HIGH : GPIO_OUTPUT_INIT_LOW;
    }

    int ret = gpio_pin_configure_dt(&self->spec, self->flags | init);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pin_init_obj, 1, pin_init);

static mp_obj_t pin_value(size_t n_args, const mp_obj_t *args) {
    pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (n_args == 1) {
        int val = gpio_pin_get_dt(&self->spec);
        if (val < 0) {
            mp_raise_OSError(-val);
        }
        return mp_obj_new_int(val);
    } else {
        int val = mp_obj_get_int(args[1]);
        int ret = gpio_pin_set_dt(&self->spec, val);
        if (ret != 0) {
            mp_raise_OSError(-ret);
        }
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_value_obj, 1, 2, pin_value);

static mp_obj_t pin_off(mp_obj_t self_in) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = gpio_pin_set_dt(&self->spec, 0);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pin_off_obj, pin_off);

static mp_obj_t pin_on(mp_obj_t self_in) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = gpio_pin_set_dt(&self->spec, 1);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pin_on_obj, pin_on);

static mp_obj_t pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_hard };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_INT_EDGE_BOTH} },
        { MP_QSTR_hard, MP_ARG_BOOL, {.u_bool = false} },
    };
    
    pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (self->irq == NULL) {
        pin_irq_obj_t *irq;
        for (irq = MP_STATE_PORT(machine_pin_irq_list); irq != NULL; irq = irq->next) {
            pin_obj_t *irq_pin = MP_OBJ_TO_PTR(irq->base.parent);
            if (irq_pin->spec.port == self->spec.port && irq_pin->spec.pin == self->spec.pin) {
                break;
            }
        }
        if (irq == NULL) {
            irq = m_new_obj(pin_irq_obj_t);
            mp_irq_init(&irq->base, &pin_irq_methods, MP_OBJ_FROM_PTR(self));
            irq->next = MP_STATE_PORT(machine_pin_irq_list);
            gpio_init_callback(&irq->callback, gpio_callback_handler, BIT(self->spec.pin));
            int ret = gpio_add_callback(self->spec.port, &irq->callback);
            if (ret != 0) {
                mp_raise_OSError(-ret);
            }
            MP_STATE_PORT(machine_pin_irq_list) = irq;
        }
        self->irq = irq;
    }

    if (n_args > 1 || kw_args->used != 0) {
        int ret = gpio_pin_interrupt_configure_dt(&self->spec, GPIO_INT_DISABLE);
        if (ret != 0) {
            mp_raise_OSError(-ret);
        }

        self->irq->base.handler = args[ARG_handler].u_obj;
        self->irq->base.ishard = args[ARG_hard].u_bool;

        if (args[ARG_handler].u_obj != mp_const_none) {
            ret = gpio_pin_interrupt_configure_dt(&self->spec, args[ARG_trigger].u_int);
            if (ret != 0) {
                mp_raise_OSError(-ret);
            }
        }
    }

    return MP_OBJ_FROM_PTR(self->irq);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pin_irq_obj, 1, pin_irq);

static void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pin_obj_t *self = self_in;
    mp_printf(print, "<Pin %s %d>", self->spec.port, self->spec.pin);
}

static mp_uint_t pin_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (new_trigger == 0) {
        new_trigger = GPIO_INT_DISABLE;
    }
    int ret = gpio_pin_interrupt_configure_dt(&self->spec, new_trigger);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    return 0;
}

static mp_uint_t pin_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (info_type == MP_IRQ_INFO_FLAGS) {
        return gpio_get_pending_int(self->spec.port);
    } else if (info_type == MP_IRQ_INFO_TRIGGERS) {
        return 0; 
    }
    return 0;
}

static const mp_irq_methods_t pin_irq_methods = {
    .trigger = pin_irq_trigger,
    .info = pin_irq_info,
};

static const mp_rom_map_elem_t pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&pin_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(PIN_MODE_IN) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(PIN_MODE_OUT) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(PIN_MODE_OPEN_DRAIN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(PIN_MODE_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(PIN_MODE_PULL_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(PIN_MODE_IRQ_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(PIN_MODE_IRQ_FALLING) },
};
static MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, pin_make_new,
    print, pin_print,
    locals_dict, &pin_locals_dict
);

static const mp_rom_map_elem_t pin_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_Pin) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&pin_type) },
};
static MP_DEFINE_CONST_DICT(pin_module_globals, pin_module_globals_table);

const mp_obj_module_t pin_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pin_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_Pin, pin_module);

MP_REGISTER_ROOT_POINTER(void *machine_pin_irq_list);