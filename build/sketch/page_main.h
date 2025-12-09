#line 1 "C:\\Users\\orangemaze\\Antigravity\\flatcat_2.0\\page_main.h"
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
      <td>
        <h3 id="title_1">Loading...</h3>
        
        <h2>Lens Cap 1</h2>
        <div class="button-container">
          <button id="servoToggle_1" class="button" onclick="toggleServo(1, openAngleJS, closeAngleJS)">Loading...</button>
        </div>
        
        <h2>Flat Panel 1</h2>
        <div class="slider-container">
          <input type="range" min="0" max="64" value="0" class="slider" id="brightnessSlider_1" oninput="handleSlider(1, this.value)">
          <span id="sliderValue_1">0</span>
        </div>
      </td>
      
      <td>
        <h3 id="title_2">Loading...</h3>
        
        <h2>Lens Cap 2</h2>
        <div class="button-container">
          <button id="servoToggle_2" class="button" onclick="toggleServo(2, openAngleJS, closeAngleJS)">Loading...</button>
        </div>
        
        <h2>Flat Panel 2</h2>
        <div class="slider-container">
          <input type="range" min="0" max="64" value="0" class="slider" id="brightnessSlider_2" oninput="handleSlider(2, this.value)">
          <span id="sliderValue_2">0</span>
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
  const slider_2 = document.getElementById('brightnessSlider_2');
  const output_2 = document.getElementById('sliderValue_2');
  const servoButton_1 = document.getElementById('servoToggle_1');
  const servoButton_2 = document.getElementById('servoToggle_2');
  const errorLock = document.getElementById('error-lock');
  const title_1 = document.getElementById('title_1');
  const title_2 = document.getElementById('title_2');
  const clockDisplay = document.getElementById('clock');

  // --- Global State Variables ---
  let currentServoState_1 = closeAngleJS; 
  let currentServoState_2 = closeAngleJS;
  let isDimmerOn = false;

  // --- Page Load ---
  window.onload = function() {
    // Fetch all statuses at once
    fetch('/getallstatus')
      .then(response => response.json())
      .then(data => {
        currentServoState_1 = data.servo1;
        updateButtonState(1, currentServoState_1);
        currentServoState_2 = data.servo2;
        updateButtonState(2, currentServoState_2);
        slider_1.value = data.dimmer1;
        output_1.innerHTML = data.dimmer1;
        slider_2.value = data.dimmer2;
        output_2.innerHTML = data.dimmer2;
        checkDimmerLock();
      });
      
    // Fetch settings for titles
    fetch('/getsettings')
      .then(response => response.json())
      .then(data => {
        title_1.innerHTML = data.title1;
        title_2.innerHTML = data.title2;
      });
      
    // Start the clock
    updateClock(); // Call it once immediately
    setInterval(updateClock, 1000); // Then update every second
  };
  
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
    else { output_2.innerHTML = value; }
    checkDimmerLock();
    // Use single-quotes for JS strings
    fetch('/slider' + sliderNum + '?value=' + value);
  }
  
  function checkDimmerLock() {
    isDimmerOn = (slider_1.value > 0 || slider_2.value > 0);
    if (isDimmerOn) {
      servoButton_1.classList.add('disabled');
      servoButton_2.classList.add('disabled');
      errorLock.style.display = 'block';
    } else {
      servoButton_1.classList.remove('disabled');
      servoButton_2.classList.remove('disabled');
      errorLock.style.display = 'none';
    }
  }

  // --- Servo Functions ---
  function updateButtonState(servoNum, angle) {
    let button = (servoNum == 1) ? servoButton_1 : servoButton_2;
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
    let currentState = (servoNum == 1) ? currentServoState_1 : currentServoState_2;
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
    if (servoNum == 1) { currentServoState_1 = newState; }
    else { currentServoState_2 = newState; }
    updateButtonState(servoNum, newState);
  }
</script>
</body>
</html>
)rawliteral";
