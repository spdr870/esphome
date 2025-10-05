from esphome import codegen as cg, config_validation as cv
from esphome.const import CONF_ID
import esphome.config_validation as cv

touch_ns = cg.esphome_ns.namespace('touch_panel')
TouchPanel = touch_ns.class_('Panel', cg.Component)

CONF_TFT_CS = "tft_cs"
CONF_TOUCH_CS = "touch_cs"
CONF_TOUCH_IRQ = "touch_irq"
CONF_COLS = "cols"
CONF_ROWS = "rows"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TouchPanel),
    cv.Required(CONF_TFT_CS): cv.int_,
    cv.Required(CONF_TOUCH_CS): cv.int_,
    cv.Required(CONF_TOUCH_IRQ): cv.int_,
    cv.Required(CONF_COLS): cv.int_,
    cv.Required(CONF_ROWS): cv.int_,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID],
                           config[CONF_TFT_CS],
                           config[CONF_TOUCH_CS],
                           config[CONF_TOUCH_IRQ],
                           config[CONF_COLS],
                           config[CONF_ROWS])
    await cg.register_component(var, config)
