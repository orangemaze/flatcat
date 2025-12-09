// ================================================================
// --- SETTINGS PAGE HTML (UPDATED with Time Zone Offsets) ---
// ================================================================
const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Device Settings</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; }
    body { max-width: 600px; margin: 0px auto; padding: 15px; }
    h1 { text-align: center; }
    form { border: 1px solid #ccc; border-radius: 8px; padding: 20px; }
    div { margin-bottom: 15px; }
    label { display: block; font-weight: bold; margin-bottom: 5px; }
    input[type="text"], select { 
      width: 98%; padding: 8px; font-size: 1.1em; 
      border: 1px solid #ddd; border-radius: 4px;
    }
    button { padding: 10px 20px; font-size: 1.1em; background-color: #007BFF; color: white; border: none; border-radius: 4px; cursor: pointer; }
    a { color: #007BFF; text-decoration: none; }
    hr { border: 0; border-top: 1px solid #eee; margin: 20px 0; }
  </style>
</head>
<body>
  <h1>Device Settings</h1>
  <form action="/save" method="POST">
    <div>
      <label for="title1">Column 1 Title</label>
      <input type="text" id="title1" name="title1">
    </div>
    <div>
      <label for="title2">Column 2 Title</label>
      <input type="text" id="title2" name="title2">
    </div>
    <hr>
    <div>
      <label for="gmtOffset">Time Zone (Standard Offset)</label>
      <select id="gmtOffset" name="gmtOffset">
        <option value="-43200">UTC-12</option>
        <option value="-39600">UTC-11</option>
        <option value="-36000">UTC-10 (Hawaii)</option>
        <option value="-32400">UTC-9 (Alaska)</option>
        <option value="-28800">UTC-8 (Pacific)</option>
        <option value="-25200">UTC-7 (Mountain)</option>
        <option value="-21600">UTC-6 (Central)</option>
        <option value="-18000">UTC-5 (Eastern)</option>
        <option value="-14400">UTC-4 (Atlantic)</option>
        <option value="-10800">UTC-3</option>
        <option value="0">UTC (0)</option>
        <option value="3600">UTC+1 (Central Europe)</option>
        <option value="7200">UTC+2</option>
      </select>
    </div>
    <div>
      <label for="daylightOffset">Daylight Saving</label>
      <select id="daylightOffset" name="daylightOffset">
        <option value="3600">On (Add 1 hour for DST)</option>
        <option value="0">Off (No DST)</option>
      </select>
    </div>
    <hr>
    <div>
      <label for="hostname">Hostname</label>
      <input type="text" id="hostname" name="hostname">
    </div>
    <div>
      <label for="ip">Static IP</label>
      <input type="text" id="ip" name="ip">
    </div>
    <div>
      <label for="gateway">Gateway</label>
      <input type="text" id="gateway" name="gateway">
    </div>
    <div>
      <label for="subnet">Subnet</label>
      <input type="text" id="subnet" name="subnet">
    </div>
    <button type="submit">Save and Reboot</button>
  </form>
  <br>
  <a href="/">Back to Home</a>

  <script>
    window.onload = function() {
      fetch('/getsettings')
        .then(response => response.json())
        .then(data => {
          document.getElementById('title1').value = data.title1;
          document.getElementById('title2').value = data.title2;
          document.getElementById('gmtOffset').value = data.gmtOffset;
          document.getElementById('daylightOffset').value = data.daylightOffset;
          document.getElementById('hostname').value = data.hostname;
          document.getElementById('ip').value = data.ip;
          document.getElementById('gateway').value = data.gateway;
          document.getElementById('subnet').value = data.subnet;
        })
        .catch(error => console.error('Error fetching settings:', error));
    };
  </script>
</body>
</html>
)rawliteral";
