"use strict";

(async () => {
    let inputs = {
        "pwm_fan_channel":      document.getElementById("pwm_fan_channel"),
        "pwm_fan_frequency":    document.getElementById("pwm_fan_frequency"),
        "pwm_fan_gpio":         document.getElementById("pwm_fan_gpio"),
        // "pwm_fan_duty":         document.getElementById("pwm_fan_duty"),
        "pwm_mos_channel":      document.getElementById("pwm_mos_channel"),
        "pwm_mos_frequency":    document.getElementById("pwm_mos_frequency"),
        "pwm_mos_gpio":         document.getElementById("pwm_mos_gpio"),
        // "pwm_mos_duty":         document.getElementById("pwm_mos_duty"),
        "wifi_ssid":            document.getElementById("wifi_ssid"),
        "wifi_password":        document.getElementById("wifi_password"),
        "wifi_channel":         document.getElementById("wifi_channel"),
        "dhcps_ip":             document.getElementById("dhcps_ip"),
        "dhcps_netmask":        document.getElementById("dhcps_netmask"),
        "dhcps_as_router":      document.getElementById("dhcps_as_router"),
    }
    for (let key in inputs) {
        // skip loop if the property is from prototype
        if (!inputs.hasOwnProperty(key)) {
            continue;
        }
        if (!inputs[key]) {
            console.error("failed to get input", key)
            return;
        }
    }
    console.log("inputs:", inputs);

    let settings = {};
    try {
        let settings_response = await fetch("/settings");
        settings = await settings_response.json()
    } catch(e) {
        console.error(e)
    }
    console.log("settings:", settings);

    for (let key in inputs) {
        if (!inputs.hasOwnProperty(key)) {
            continue;
        }
        inputs[key].value = settings[key];
    }

    let button_save = document.getElementById("button_save");
    let failed_message = document.getElementById("failed_message");
    button_save.addEventListener("click", async () => {
        failed_message.textContent = "";
        let { ok, msg } = is_valid_config(inputs);
        if (!ok) {
            failed_message.textContent = "FAILED: " + msg;
            console.error("invalid config, msg: ", msg);
            return;
        }
        button_save.textContent = "Saving...";
        let query = "?"
        for (let key in inputs) {
            if (!inputs.hasOwnProperty(key)) {
                continue;
            }
            query += key + "=" + inputs[key].value + "&";
        }
        let query_url = "http://" + window.location.host + "/settings" + query;
        try {
            await fetch(query_url);
        } catch(e) {
            console.error(e);
            alert("FAILED to apply settings: " + e);
            return;
        }
        alert("Settings saved");
        location.reload();
    });
})();

function is_valid_config(inputs) {
    let pwm_fan_channel = parseInt(inputs["pwm_fan_channel"].value);
    if (isNaN(pwm_fan_channel) || pwm_fan_channel > 4 || pwm_fan_channel < 0) {
        return {
            ok: false,
            msg: "Invalid PWM FAN CHANNEL: " + pwm_fan_channel,
        }
    }
    let pwm_fan_frequency = parseInt(inputs["pwm_fan_frequency"].value);
    if (isNaN(pwm_fan_frequency) || pwm_fan_frequency > 30000 ||
            pwm_fan_frequency < 1000) {
        return {
            ok: false,
            msg: "Invalid PWM FAN FREQUENCY: " + pwm_fan_channel,
        }
    }
    let pwm_fan_gpio = parseInt(inputs["pwm_fan_gpio"].value);
    if (isNaN(pwm_fan_gpio) || pwm_fan_gpio > 42 || pwm_fan_gpio < 0) {
        return {
            ok: false,
            msg: "Invalid PWM FAN GPIO: " + pwm_fan_gpio,
        }
    }
    let pwm_mos_channel = parseInt(inputs["pwm_mos_channel"].value);
    if (isNaN(pwm_mos_channel) || pwm_mos_channel > 4 || pwm_mos_channel < 0) {
        return {
            ok: false,
            msg: "Invalid LED CHANNEL: " + pwm_mos_channel,
        }
    }
    let pwm_mos_frequency = parseInt(inputs["pwm_mos_frequency"].value);
    if (isNaN(pwm_mos_frequency) || pwm_mos_frequency > 30000 ||
    pwm_mos_frequency < 1000) {
        return {
            ok: false,
            msg: "Invalid LED FREQUENCY: " + pwm_fan_channel,
        }
    }
    let pwm_mos_gpio = parseInt(inputs["pwm_mos_gpio"].value);
    if (isNaN(pwm_mos_gpio) || pwm_mos_gpio > 42 || pwm_mos_gpio < 0) {
        return {
            ok: false,
            msg: "Invalid LED GPIO: " + pwm_mos_gpio,
        }
    }
    let wifi_ssid = inputs["wifi_ssid"].value;
    if (wifi_ssid.length > 20) {
        return {
            ok: false,
            msg: "WIFI SSID length too long (should <= 20)",
        }
    }
    if (wifi_ssid.length < 3) {
        return {
            ok: false,
            msg: "WIFI SSID length too short",
        }
    }
    let wifi_password = inputs["wifi_password"].value;
    if (wifi_password.length > 20) {
        return {
            ok: false,
            msg: "WIFI Password length too long (should <= 20)",
        }
    }
    if (wifi_password.length < 8) {
        return {
            ok: false,
            msg: "WIFI Password length too short (should >= 8)",
        }
    }
    let wifi_channel = parseInt(inputs["wifi_channel"].value);
    if (isNaN(wifi_channel) || wifi_channel > 11 || wifi_channel < 1) {
        return {
            ok: false,
            msg: "Invalid WIFI Channel: " + wifi_channel,
        }
    }
    let dhcps_ip = inputs["dhcps_ip"].value;
    if (!is_valid_ip(dhcps_ip, false)) {
        return {
            ok: false,
            msg: "Invalid DHCP Server IP: " + dhcps_ip,
        }
    }
    let dhcps_netmask = inputs["dhcps_netmask"].value;
    if (!is_valid_ip(dhcps_netmask, false)) {
        return {
            ok: false,
            msg: "Invalid DHCP Server netmask: " + dhcps_netmask,
        }
    }
    let dhcps_as_router = parseInt(inputs["dhcps_as_router"].value);
    if (isNaN(dhcps_as_router) || dhcps_as_router > 1 || wifi_channel < 0) {
        return {
            ok: false,
            msg: "AS ROUTER value shoule be 0 or 1",
        }
    }
    console.log("config validate pass");
    return {
        ok: true,
        msg: "",
    };
}

function is_valid_ip(str, is_netmask) {
    if (str.length > 15 || str.length < 7) {
        return false;
    }
    let spec = str.split(".")
    if (spec.length != 4) {
        return false;
    }
    let ip = [0, 0, 0, 0];
    for (let i = 0; i < spec.length; i++) {
        let v = parseInt(spec[i]);
        if (isNaN(v) || v > 255 || v < 0) {
            return false;
        }
        ip[i] = v;
    }
    if (ip[0] == 0) {
        return false;
    }

    if (!is_netmask) {
        return true;
    }
    if (ip[3] != 0) {
        return false;
    }
    return true;
}

async function restart() {
    let button_restart = document.getElementById("restart");
    if (button_restart) {
        button_restart.textContent = "Restarting..."
    }
    try {
        await fetch("/restart");
    } catch(e) {
        console.error(e)
        return
    }
    alert("Restart requested, reconnect to the controller WIFI "+
        "in a few seconds later manually.");
    location.href = "/";
}

async function reset_to_default() {
    try {
        await fetch("/reset_settings");
    } catch(e) {
        console.error(e)
        alert("Failed: " + e);
        return
    }
    alert("Settings have been reset.");
    location.reload();
}
