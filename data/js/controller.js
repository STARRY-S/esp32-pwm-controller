"use strict";

(async () => {
    let fan_enable = document.getElementById("fan-enable");
    let fan_speed = document.getElementById("fan-speed");
    let fan_speed_percentage = document.getElementById("fan-speed-percentage");
    let button_save = document.getElementById("button-save");
    if (!fan_enable || !fan_speed || !fan_speed_percentage || !button_save) {
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

    fan_speed.addEventListener("input", () => {
        let per = Math.round(fan_speed.value * 100 / 255);
        fan_speed_percentage.textContent = `${fan_speed.value} (${per} %)`;
    });
    fan_enable.addEventListener("input", () => {
        if (fan_enable.checked) {
            fan_speed.value = config["pwm_fan_duty"];
            fan_speed.type = "range";
            let per = Math.round(fan_speed.value * 100 / 255);
            fan_speed_percentage.textContent = `${fan_speed.value} (${per} %)`;
        } else {
            fan_speed.value = 0;
            fan_speed.type = "hidden";
            fan_speed_percentage.textContent = `0 (0%)`;
        }
    });

    let mos_duty = parseInt(config["pwm_mos_duty"]);
    if (isNaN(mos_duty) || mos_duty <= 10) {
        fan_enable.checked = false;
        fan_speed.readOnly = true;
        fan_speed.type = "hidden";
        fan_speed.value = 0;
    } else {
        fan_enable.checked = true;
        fan_speed.value = config["pwm_fan_duty"];
        fan_speed.readOnly = false;
    }

    let per = Math.round(fan_speed.value * 100 / 255);
    fan_speed_percentage.textContent = `${fan_speed.value} (${per} %)`;

    button_save.addEventListener("click", async () => {
        button_save.textContent = "Saving...";
        let query = "?";
        if (!fan_enable.checked) {
            query += "pwm_mos_duty=0";
        } else {
            query += "pwm_mos_duty=250&pwm_fan_duty=" + fan_speed.value;
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
