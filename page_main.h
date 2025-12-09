// ================================================================
// --- MAIN (HOME) PAGE HTML ---
// (2-Column Table Layout)
// ================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Flatcat Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Arial; display: inline-block; text-align: center; }
    body { max-width: 800px; margin:0px auto; padding-bottom: 25px; }
    h1 { font-size: 2.2rem; }
    h2 { font-size: 1.5rem; color: #555; margin-top: 25px; }
    h3 { font-size: 1.2rem; color: #007BFF; margin-bottom: 20px; }
    
    /* Table Layout */
    table { width: 100%; border-collapse: collapse; }
    td { width: 50%; vertical-align: top; padding: 10px; border: 1px solid #ddd; }

    /* Controls */
    .slider-container { display: flex; align-items: center; justify-content: center; margin-top: 15px; }
    .slider {
      -webkit-appearance: none; width: 200px; /* Narrower slider */
      height: 25px; background: #d3d3d3; outline: none; opacity: 0.7;
      transition: opacity .2s; border-radius: 12px;
    }
    .slider:hover { opacity: 1; }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none; appearance: none;
      width: 40px; height: 40px; background: #007BFF;
      cursor: pointer; border-radius: 50%;
    }
    #sliderValue_1, #sliderValue_2 { font-size: 2rem; font-weight: bold; margin-left: 20px; }
    
    .button-container { margin-top: 20px; }
    .button { padding: 15px 40px; font-size: 1.2rem; font-weight: bold; color: white; border: none; border-radius: 8px; cursor: pointer; width: 150px; transition: background-color 0.2s; }
    .btn-green { background-color: #28a745; }
    .btn-red { background-color: #dc3545; }
    .button.disabled { background-color: #999; cursor: not-allowed; opacity: 0.7; }
    .error-msg { color: #dc3545; font-weight: bold; margin-top: 10px; display: none; }
    
    footer { margin-top: 30px; font-size: 0.9em; }
    footer a { color: #007BFF; text-decoration: none; margin: 0 10px; }
    
    /* Clock Style */
    #clock {
      position: absolute;
      top: 10px;
      left: 10px;
      font-size: 1.1em;
      font-weight: bold;
      color: #333;
    }
  </style>
</head>
<body>
  <div id="clock">Syncing...</div>
  <h1>Flatcat Control</h1>

  <table>
    <tr>
      <td style="width: 100%;">
        <h3 id="title_1">Waiting...</h3>
        
        <h2>Lens Cap 1</h2>
        <div class="button-container">
          <button id="servoToggle_1" class="button" onclick="toggleServo(1, openAngleJS, closeAngleJS)">Waiting...</button>
        </div>
        
        <h2>Flat Panel 1</h2>
        <div class="slider-container">
          <input type="range" min="0" max="64" value="0" class="slider" id="brightnessSlider_1" oninput="handleSlider(1, this.value)">
          <span id="sliderValue_1">0</span>
        </div>
      </td>
    </tr>
  </table>
  
  <div id="error-lock" class="error-msg">Error: Turn off dimmers to move servos.</div>
  
  <footer>
    <a href="/settings">Device Settings</a>
    <a href="/api/v1/management/v1/supporteddevices" target="_blank">Alpaca Status</a>
  </footer>

<script>
  // --- Constants (Must match C++ code) ---
  const openAngleJS = 90;
  const closeAngleJS = 0;

  // --- Get UI Elements ---
  const slider_1 = document.getElementById('brightnessSlider_1');
  const output_1 = document.getElementById('sliderValue_1');
  const servoButton_1 = document.getElementById('servoToggle_1');
  const errorLock = document.getElementById('error-lock');
  const title_1 = document.getElementById('title_1');
  const clockDisplay = document.getElementById('clock');

  // --- Global State Variables ---
  let currentServoState_1 = closeAngleJS; 
  let isDimmerOn = false;

  // --- Page Load ---
  window.onload = function() {
    console.log("DEBUG: Page Loaded. Starting Fetches...");

    updateAllStatus(); // Run once immediately
    setInterval(updateAllStatus, 2000); // Poll every 2 seconds

    // Start the clock
    updateClock(); 
    setInterval(updateClock, 1000); 
      
    // Fetch settings for titles (Once is fine)
    fetch('/getsettings')
      .then(response => response.json())
      .then(data => {
        title_1.innerHTML = data.title1;
      });
  };

  function updateAllStatus() {
    // Fetch all statuses
    fetch('/getallstatus')
      .then(response => {
        // console.log("DEBUG: /getallstatus response received");
        return response.json();
      })
      .then(data => {
        // console.log("DEBUG: /getallstatus data:", data);
        
        // Use the REAL state (from sensors) to update the UI
        let realState = data.coverState; // 1=Closed, 3=Open, 4=Unknown
        updateButtonStateFromSensor(1, realState);
        
        // Only update slider if user is NOT dragging it? 
        // For now, simpler to just update, might cause jitter if active use.
        // Better: Only update if document.activeElement !== slider_1
        if (document.activeElement !== slider_1) {
             slider_1.value = data.dimmer1;
             output_1.innerHTML = data.dimmer1;
        }
        checkDimmerLock();
      })
      .catch(err => {
        console.error("ERROR fetching /getallstatus:", err);
      });
  }
  
  // --- Clock Function ---
  function updateClock() {
    fetch('/gettime')
      .then(response => response.text())
      .then(timeString => {
        clockDisplay.innerHTML = timeString;
      });
  }

  // --- Dimmer Functions ---
  function handleSlider(sliderNum, value) {
    if (sliderNum == 1) { output_1.innerHTML = value; }
    checkDimmerLock();
    // Use single-quotes for JS strings
    fetch('/slider' + sliderNum + '?value=' + value);
  }
  
  function checkDimmerLock() {
    isDimmerOn = (slider_1.value > 0);
    if (isDimmerOn) {
      servoButton_1.classList.add('disabled');
      errorLock.style.display = 'block';
    } else {
      servoButton_1.classList.remove('disabled');
      errorLock.style.display = 'none';
    }
  }

  // --- Servo Functions ---
  
  // NEW: Update UI based on ASCOM State (1=Closed, 2=Moving, 3=Open)
  function updateButtonStateFromSensor(servoNum, state) {
    let button = servoButton_1;
    if (state == 3) { // OPEN
      button.innerHTML = 'Close'; // Next Action
      button.className = 'button btn-red';
      currentServoState_1 = openAngleJS; // Sync angle variable
    } else if (state == 1 || state == 4) { // CLOSED or UNKNOWN (4) -> Allow Open
      button.innerHTML = 'Open'; 
      button.className = 'button btn-green';
      currentServoState_1 = closeAngleJS; // Sync angle variable
    } else {
      // Moving (2) or Error
      button.innerHTML = 'Moving...';
      button.className = 'button disabled';
    }
    checkDimmerLock();
  }

  // Fallback for click events (optimistic update)
  function updateButtonState(servoNum, angle) {
     // ... legacy simple toggle logic if needed, but we mostly rely on sensors now
     // For immediate click feedback:
    let button = servoButton_1;
    if (angle == openAngleJS) {
      button.innerHTML = 'Close';
      button.className = 'button btn-red';
    } else {
      button.innerHTML = 'Open';
      button.className = 'button btn-green';
    }
    checkDimmerLock();
  }

  function toggleServo(servoNum) {
    if (isDimmerOn) { return; } // Do nothing
    let currentState = currentServoState_1;
    let newState = 0;
    let command = '';
    if (currentState == openAngleJS) {
      command = '/close' + servoNum; 
      newState = closeAngleJS;
    } else {
      command = '/open' + servoNum; 
      newState = openAngleJS;
    }
    fetch(command);
    currentServoState_1 = newState;
    updateButtonState(servoNum, newState);
  }
</script>
</body>
</html>
)rawliteral";
