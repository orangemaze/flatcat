// ================================================================
// --- REBOOT PAGE HTML ---
// ================================================================
const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Rebooting...</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; }
    body { max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
    h1 { color: #007BFF; }
  </style>
  <script>
    setTimeout(function() {
      window.location.href = '/';
    }, 10000); // Wait 10 seconds for the device to reboot
  </script>
</head>
<body>
  <h1>Settings Saved</h1>
  <p>The device is now rebooting to apply the new settings.</p>
  <p>You will be redirected to the homepage in 10 seconds.</p>
  <p>If not redirected, <a href="/">click here</a>.</p>
</body>
</html>
)rawliteral";
