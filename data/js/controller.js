"use strict";

function httpGet(u) {
    let xmlHttp = new XMLHttpRequest();
    xmlHttp.open("GET", u, false);
    xmlHttp.send(null);
    return xmlHttp.responseText;
}

(() => {
    let form = document.getElementById("submit-form");
    let fanEnable = document.getElementById("fan-enable");
    let fanSpeed = document.getElementById("fan-speed");
    let fanSpeedPer = document.getElementById("fan-speed-percentage");
    if (!fanEnable || !fanSpeed || !fanEnable || !fanSpeed || !fanSpeedPer) {
        return;
    }

    form.addEventListener("submit", () => {
        if(document.getElementById("fan-enable").checked) {
            document.getElementById('fan-enable-hidden').disabled = true;
        }
    });
    fanSpeed.addEventListener("input", () => {
        let per = Math.round(fanSpeed.value * 100 / 255);
        fanSpeedPer.textContent = `${fanSpeed.value} (${per} %)`;
    });
    fanEnable.addEventListener("input", () => {
        if (fanEnable.checked) {
            fanSpeed.value = 20;
            fanSpeed.type = "range";
            let per = Math.round(fanSpeed.value * 100 / 255);
            fanSpeedPer.textContent = `${fanSpeed.value} (${per} %)`;
        } else {
            fanEnable.checked = false;
            fanSpeed.value = 0;
            fanSpeed.type = "hidden";
            fanSpeedPer.textContent = `0 (0%)`;
        }
    });

    let configText = httpGet("/controller?getconfig=1");
    let config = {
        duty: 0,
    };
    try {
        config = JSON.parse(configText);
        console.log("config: ", config);
    } catch (e) {
        console.error(e)
    }

    if (config["duty"] == 0) {
        fanEnable.checked = false;
        fanSpeed.readOnly = true;
        fanSpeed.type = "hidden";
        fanSpeed.value = 0;
    } else {
        fanEnable.checked = true;
        fanSpeed.value = config["duty"];
        fanSpeed.readOnly = false;
    }

    let per = Math.round(fanSpeed.value * 100 / 255);
    fanSpeedPer.textContent = `${fanSpeed.value} (${per} %)`;
})();
