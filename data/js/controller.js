"use strict";

(async () => {
    let fan_enable = document.getElementById("fan-enable");
    let fan_speed = document.getElementById("fan-speed");
    let fan_speed_percentage = document.getElementById("fan-speed-percentage");

    let led_enable = document.getElementById("led-enable");
    let led_brightness = document.getElementById("led-brightness");
    let led_percentage = document.getElementById("led-percentage");
    let button_save = document.getElementById("button-save");
    if (!fan_enable || !fan_speed || !fan_speed_percentage ||
        !led_enable || !led_brightness || !led_percentage || !button_save) {
        return;
    }

    let config = {};
    try {
        let settings_response = await fetch("/settings");
        config = await settings_response.json()
    } catch (e) {
        console.error(e)
        return
    }

    const FAN_MIN = parseInt(config["pwm_fan_duty_min"])
    const FAN_MAX = parseInt(config["pwm_fan_duty_max"])
    const LED_MIN = parseInt(config["pwm_mos_duty_min"])
    const LED_MAX = parseInt(config["pwm_mos_duty_max"])

    fan_speed.min = FAN_MIN;
    fan_speed.max = FAN_MAX;
    led_brightness.min = LED_MIN;
    led_brightness.max = LED_MAX;

    fan_speed.addEventListener("input", () => {
        fan_speed_percentage.textContent = get_percentage(FAN_MIN, FAN_MAX, fan_speed.value);
    });
    fan_enable.addEventListener("input", () => {
        if (fan_enable.checked) {
            fan_speed.value = 80;
            fan_speed.type = "range";
            fan_speed_percentage.textContent = get_percentage(FAN_MIN, FAN_MAX, 80);
        } else {
            fan_speed.value = 0;
            fan_speed.type = "hidden";
            fan_speed_percentage.textContent = `N/A`;
        }
    });

    led_brightness.addEventListener("input", () => {
        led_percentage.textContent = get_percentage(LED_MIN, LED_MAX, led_brightness.value);
    })
    led_enable.addEventListener("input", () => {
        if (led_enable.checked) {
            led_brightness.value = 28;
            led_brightness.type = "range";
            led_percentage.textContent = get_percentage(LED_MIN, LED_MAX, 28);
        } else {
            led_brightness.value = 0;
            led_brightness.type = "hidden";
            led_percentage.textContent = `N/A`;
        }
    })

    let fan_duty = parseInt(config["pwm_fan_duty"]);
    let led_duty = parseInt(config["pwm_mos_duty"]);
    if (isNaN(fan_duty) || fan_duty <= 1) {
        fan_enable.checked = false;
        fan_speed.type = "hidden";
        fan_speed.value = 0;
    } else {
        fan_enable.checked = true;
        fan_speed.value = fan_duty;
        fan_speed_percentage.textContent = get_percentage(FAN_MIN, FAN_MAX, fan_duty);
    }
    if (isNaN(led_duty) || led_duty <= 1) {
        led_enable.checked = false;
        led_brightness.type = "hidden";
        led_brightness.value = 0;
    } else {
        led_enable.checked = true;
        led_brightness.value = led_duty;
        led_percentage.textContent = get_percentage(LED_MIN, LED_MAX, led_duty);
    }

    button_save.addEventListener("click", async () => {
        button_save.textContent = "Saving...";
        let query = "?";
        if (!fan_enable.checked) {
            query += "pwm_fan_duty=0&";
        } else {
            query += "pwm_fan_duty=" + fan_speed.value + "&";
        }
        if (!led_enable.checked) {
            query += "pwm_mos_duty=0";
        } else {
            query += "pwm_mos_duty=" + led_brightness.value + "&";
        }
        try {
            let query_url = "http://" + window.location.host + "/settings" + query;
            console.log("query: ", query);
            await fetch(query_url);
        } catch(e) {
            console.error(e);
            alert("FAILED: " + e);
        }

        location.reload();
    });
})();

function get_percentage(min, max, value) {
    min = parseInt(min);
    max = parseInt(max);
    value = parseInt(value);
    let per = Math.round((value - min) * 100.0 / (max - min));
    return `${value} (${per} %)`;
}
