"use strict";

function httpGet(u)
{
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open("GET", u, false); // false for synchronous request
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
        fanSpeedPer.textContent = Math.round(
            (fanSpeed.value / 255) * 100) + "%";
    });

    let configText = httpGet("/controller?getconfig=1");
    let config = JSON.parse(configText);
    console.log("config: ", config);

    if (config["duty"] == 0) {
        fanEnable.checked = false;
        fanSpeed.value = 0;
        fanSpeed.readOnly = true;
    } else {
        fanEnable.checked = true;
        fanSpeed.value = config["duty"];
        fanSpeed.readOnly = false;
    }

    fanSpeedPer.textContent = Math.round((config["duty"] / 255) * 100) + "%";
})();
