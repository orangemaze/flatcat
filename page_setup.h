// ================================================================
// --- SETUP (AP MODE) PAGE HTML ---
// ================================================================
const char setup_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Setup flatcat</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; }
    body { max-width: 600px; margin: 0px auto; padding: 15px; background: #f4f4f4; }
    h1 { text-align: center; color: #333; }
    form { border: 1px solid #ccc; border-radius: 8px; padding: 20px; background: #fff; }
    div { margin-bottom: 15px; }
    label { display: block; font-weight: bold; margin-bottom: 5px; }
    input[type="text"], input[type="password"], select {
      width: 95%; padding: 10px; font-size: 1.1em; border: 1px solid #ddd; border-radius: 4px;
    }
    button {
      padding: 10px 20px; font-size: 1.1em; background-color: #007BFF;
      color: white; border: none; border-radius: 4px; cursor: pointer;
    }
    #scan-btn { background-color: #6c757d; }
  </style>
</head>
<body>
  <h1>Configure Wi-Fi</h1>
  <form action="/savewifi" method="POST">
    <div>
      <label for="ssid">Wi-Fi Network (SSID)</label>
      <select id="ssid" name="ssid" onchange="document.getElementById('ssid-text').value=this.value">
        <option value="">Select a Network</option>
      </select>
      <br><br>
      <button type="button" id="scan-btn" onclick="scanNetworks()">Scan for Networks</button>
      <br><br>
      <input type="text" id="ssid-text" name="ssid_text" placeholder="Or type SSID if hidden">
    </div>
    <div>
      <label for="pass">Password</label>
      <input type="password" id="pass" name="pass">
    </div>
    <button type="submit">Save and Connect</button>
  </form>

  <script>
    function scanNetworks() {
      var scanBtn = document.getElementById('scan-btn');
      scanBtn.innerHTML = 'Scanning...';
      scanBtn.disabled = true;
      
      fetch('/scan')
        .then(response => response.json())
        .then(networks => {
          var select = document.getElementById('ssid');
          select.innerHTML = '<option value="">Select a Network</option>'; // Clear old
          networks.forEach(net => {
            var opt = document.createElement('option');
            opt.value = net.ssid;
            opt.innerHTML = net.ssid + ' (' + net.rssi + ' dBm, ' + net.enc + ')';
            select.appendChild(opt);
          });
          scanBtn.innerHTML = 'Scan for Networks';
          scanBtn.disabled = false;
        })
        .catch(error => {
          console.error('Error scanning:', error);
          scanBtn.innerHTML = 'Scan Failed (Retry)';
          scanBtn.disabled = false;
        });
    }
  </script>
</body>
</html>
)rawliteral";
