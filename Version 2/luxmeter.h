const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Lux Scribe</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="/favicon.svg">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  p {
    margin-block-start: 0.5em;
    margin-block-end: 0.5em;
  }
  input {
    color: #143642;
    background-color: #fafafa;
    border: 1px solid #ccc;
    border-radius: 0;
    padding: 10px 15px;
    box-sizing: border-box;
    max-width: 50px;
  }
  .content {
    padding: 20px 20px 10px 20px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:10px;
  }
  .button {
    padding: 10px 16px;
    margin: 5px;
    font-size: 16px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>Lux Scribe</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>Lux Scribe &#x1F58B;&#xFE0F;</h1>
  </div>
  <div class="content">
    <div class="card">
      <p class="state">
        Lumens: <span id="lumens">%LUMENS%</span>
        <br>
        Temperature: <span id="temp">%TEMP%</span>&deg;C
        <br>
        Status: <span id="logMsg">stopped</span>
      </p>
      <p>
        <button id="startStop" class="button">Start Logging</button>
        <button id="clearLog" class="button">Clear Log</button>
        <button id="downloadCSV" class="button">Download CSV</button>
        <button id="copyToClipboard" class="button">Copy to Clipboard</button>
        <button id="setDivisor" class="button">Set Divisor</button>
        <br>
        Minimum Lumens to log: <input type="text" id="minLumens" value="3">
      </p>
    </div>
  </div>
  <div class="content">
    <div class="card" id="chartDiv"></div>
  </div>
  <div class="content">
    <div class="card" id="log" style="white-space: pre;">Minutes, Lumens, Temperature</div>
  </div>
<script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  var start_time = null;
  var log_flag = 0;
  var waiting_for_light;
  var minimum_lumens = 3;
  var low_lumens_count;
  window.addEventListener('load', onLoad);
  window.addEventListener('beforeunload', onBeforeUnload);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var lumens = event.data.split(',')[0];
    var temp = event.data.split(',')[1];
    document.getElementById('lumens').innerHTML = lumens;
    document.getElementById('temp').innerHTML = temp;
    
    if(waiting_for_light && lumens >= minimum_lumens) { 
      waiting_for_light = 0; 
      document.getElementById('logMsg').innerHTML = 'logging';
    }
    if(log_flag && !waiting_for_light) {
      if(start_time === null) start_time = new Date();
      var current_time = new Date();
      var elapsed_millis = current_time - start_time;
      var current_minutes = (elapsed_millis / 1000 / 60).toFixed(3);
      
      document.getElementById('log').append("\n" + current_minutes + ', ' + event.data);
      data.addRow([parseFloat(current_minutes),parseFloat(lumens),parseFloat(temp)]);
      drawChart();
      
      if(lumens < minimum_lumens) low_lumens_count++;
      else low_lumens_count = 0;
      if(low_lumens_count >= 3) startStop();
    }

  }
  function onLoad(event) {
    initWebSocket();
    initButtons();
  }
  function onBeforeUnload(e) { // make sure user saved their work before closing or reloading
    e.preventDefault();
    e.returnValue = '';
  }
  function initButtons() {
    document.getElementById('startStop').addEventListener('click', startStop);
    document.getElementById('clearLog').addEventListener('click', clearLog);
    document.getElementById('downloadCSV').addEventListener('click', downloadCSV);
    document.getElementById('copyToClipboard').addEventListener('click', copyToClipboard);
    document.getElementById('setDivisor').addEventListener('click', setDivisor);
  }
  function startStop() {
    if(log_flag) {
      log_flag = 0;
      document.getElementById('startStop').innerHTML = 'Start Logging';
      document.getElementById('logMsg').innerHTML = 'stopped';
    }
    else {
      log_flag = 1;
      waiting_for_light = 1;
      low_lumens_count = 0;
      var ml = parseFloat( document.getElementById('minLumens').value );
      if(!isNaN(ml)) { minimum_lumens = ml; }
      document.getElementById('startStop').innerHTML = 'Stop Logging';
      document.getElementById('logMsg').innerHTML = 'waiting for light';
      websocket.send('update'); // request an update
    }
  }
  function clearLog() {
    var ok_to_clear = confirm("Are you sure you want to clear the log? Press Ok to clear it; press Cancel to... cancel.");
    if (ok_to_clear) { 
      document.getElementById('log').innerHTML = 'Minutes, Lumens, Temperature'; 
      start_time = null;
      initChart();
    }
  }
  function downloadCSV() {
    var a = document.body.appendChild( document.createElement("a") );
    a.download = "LuxLog.csv";
    a.href = "data:text/csv," + encodeURI(document.getElementById("log").innerHTML);
    a.click();
  }
  function copyToClipboard() {
    var logdiv = document.getElementById("log");
    var selection = window.getSelection();
    var range = document.createRange();
    range.selectNodeContents(logdiv);
    selection.removeAllRanges();
    selection.addRange(range);
    document.execCommand("Copy");
    selection.removeAllRanges();
  }
  function setDivisor() {
    var divisor = parseFloat(prompt('Enter a new divisor'));
    // if user provided a valid number, send it to the lux meter
    if(isFinite(divisor) && divisor > 0) { websocket.send('divisor:'+divisor); }
  }

  google.charts.load('current', {'packages':['corechart']});
  google.charts.setOnLoadCallback(initChart);

  var data;
  var chart;
  var options = {
    'backgroundColor': '#F8F7F9',
    series: {
      0: {targetAxisIndex: 0},
      1: {targetAxisIndex: 1}
    },
    vAxes: {
      0: {title: 'Lumens'},
      1: {title: 'Temperature \u00B0C'}
    },
    legend: {
      position: 'none'
    }
  };

  function drawChart() {
    chart.draw(data, options);
  }

  function initChart() {
    data = new google.visualization.DataTable();
    data.addColumn('number', 'Minutes');
    data.addColumn('number', 'Lumens');
    data.addColumn('number', 'Temperature \u00B0C');
    chart = new google.visualization.LineChart(document.getElementById('chartDiv'));
    data.addRow([0.0,0.0,0.0]);
    drawChart();
  }

</script>
</body>
</html>
)rawliteral";

const char favicon_svg[] PROGMEM = R"rawliteral(
<svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
   viewBox="0 0 372.278 372.278" style="enable-background:new 0 0 372.278 372.278;" xml:space="preserve">
<style>
  path { fill: #000000; }
  @media (prefers-color-scheme: dark) { path { fill: #ffffff; } }
</style>
<g id="XMLID_836_">
  <path id="XMLID_837_" d="M209.454,56.757c-5.856-5.858-15.355-5.857-21.213,0l-42.428,42.426c-0.046,0.045-0.085,0.097-0.13,0.143
    c-0.256,0.263-0.502,0.534-0.738,0.815c-0.061,0.073-0.124,0.143-0.184,0.217c-0.271,0.335-0.527,0.681-0.77,1.039
    c-0.052,0.077-0.1,0.156-0.149,0.233c-0.196,0.303-0.382,0.614-0.558,0.931c-0.049,0.089-0.099,0.176-0.146,0.266
    c-0.2,0.383-0.387,0.774-0.555,1.175c-0.027,0.064-0.049,0.13-0.075,0.194c-0.138,0.345-0.263,0.695-0.376,1.052
    c-0.035,0.109-0.07,0.219-0.102,0.329c-0.123,0.417-0.234,0.84-0.32,1.271l-13.25,66.258l-36.295,36.295
    c-5.574-2.66-12.445-1.698-17.062,2.919L46.82,240.606c-4.616,4.616-5.579,11.487-2.919,17.062L4.394,297.174
    c-5.858,5.858-5.858,15.355,0,21.213l49.496,49.496c2.929,2.929,6.768,4.394,10.606,4.394c3.839,0,7.678-1.464,10.606-4.394
    L199.17,243.816l66.258-13.25c0.432-0.087,0.854-0.197,1.271-0.32c0.11-0.032,0.22-0.067,0.328-0.102
    c0.357-0.113,0.709-0.238,1.053-0.377c0.065-0.026,0.13-0.048,0.193-0.075c0.401-0.168,0.793-0.355,1.176-0.556
    c0.089-0.046,0.176-0.097,0.265-0.145c0.318-0.175,0.63-0.361,0.933-0.559c0.078-0.05,0.156-0.097,0.232-0.148
    c0.358-0.242,0.705-0.499,1.04-0.77c0.072-0.059,0.142-0.122,0.214-0.181c0.283-0.238,0.557-0.486,0.821-0.745
    c0.045-0.043,0.095-0.081,0.139-0.126l42.428-42.428c2.814-2.813,4.394-6.628,4.394-10.606c0-3.979-1.58-7.793-4.394-10.607
    L209.454,56.757z M64.496,336.063L36.213,307.78l106.066-106.066l28.283,28.283L64.496,336.063z M196.706,213.714l-38.144-38.144
    l7.07-35.355l66.428,66.428L196.706,213.714z M262.487,194.643l-84.853-84.853l21.214-21.213l84.853,84.852L262.487,194.643z"/>
  <path id="XMLID_842_" d="M357.278,104.816h-21.125c-8.284,0-15,6.716-15,15c0,8.284,6.716,15,15,15h21.125c8.284,0,15-6.716,15-15
    C372.278,111.531,365.562,104.816,357.278,104.816z"/>
  <path id="XMLID_843_" d="M252.46,51.125L252.46,51.125c8.284,0,15-6.715,15.001-14.999l0.002-21.124
    c0.001-8.284-6.714-15-14.999-15.002h-0.001c-8.283,0-14.999,6.715-15,14.999l-0.002,21.124
    C237.46,44.408,244.175,51.124,252.46,51.125z"/>
  <path id="XMLID_844_" d="M311.641,75.637c3.838,0,7.678-1.464,10.606-4.393l14.938-14.936c5.857-5.858,5.858-15.355,0.001-21.213
    c-5.858-5.857-15.355-5.859-21.214,0L301.034,50.03c-5.857,5.858-5.858,15.355-0.001,21.213
    C303.963,74.173,307.802,75.637,311.641,75.637z"/>
</g>
</svg>
)rawliteral";
