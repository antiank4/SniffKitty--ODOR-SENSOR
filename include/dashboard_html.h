#ifndef DASHBOARD_HTML_H
#define DASHBOARD_HTML_H

const char DASHBOARD_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>SniffKitty Dashboard</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    body { margin: 0; font-family: Arial, sans-serif; background: #f3f2ee; color: #1f2937; }
    body::before { content: ""; position: fixed; inset: 0; z-index: -2; background: radial-gradient(circle at 18% 22%, rgba(37,99,235,0.16), transparent 28%), radial-gradient(circle at 84% 18%, rgba(22,163,74,0.12), transparent 26%), radial-gradient(circle at 68% 82%, rgba(180,83,9,0.14), transparent 28%), linear-gradient(135deg, #f7f5ef, #edf4f7 48%, #f6f1f4); animation: ambientShift 16s ease-in-out infinite alternate; }
    body::after { content: ""; position: fixed; inset: 0; z-index: -1; opacity: 0.28; pointer-events: none; background-image: linear-gradient(rgba(17,24,39,0.045) 1px, transparent 1px), linear-gradient(90deg, rgba(17,24,39,0.04) 1px, transparent 1px); background-size: 42px 42px; animation: gridDrift 22s linear infinite; }
    @keyframes ambientShift { from { filter: hue-rotate(0deg) saturate(1); transform: scale(1); } to { filter: hue-rotate(10deg) saturate(1.1); transform: scale(1.04); } }
    @keyframes gridDrift { from { background-position: 0 0, 0 0; } to { background-position: 84px 42px, 84px 42px; } }
    @keyframes cardReveal { 0% { opacity: 0; transform: translateY(26px) scale(0.96); filter: blur(5px); } 70% { opacity: 1; transform: translateY(-2px) scale(1.01); filter: blur(0); } 100% { opacity: 1; transform: translateY(0) scale(1); filter: blur(0); } }
    @keyframes pulseGlow { 0%, 100% { box-shadow: 0 0 0 rgba(239,68,68,0); } 50% { box-shadow: 0 0 24px rgba(239,68,68,0.2); } }
    .center-page { min-height: 100vh; display: flex; justify-content: center; align-items: center; padding: 24px; }
    .login-card { width: 330px; background: rgba(255,255,255,0.92); border-radius: 28px; padding: 28px; box-shadow: 0 20px 50px rgba(0,0,0,0.12); text-align: center; }
    .login-card h1 { margin: 0; font-size: 30px; font-weight: 800; }
    .subtitle { color: #6b7280; font-size: 14px; margin: 10px 0 22px; }
    .cat-preview { width: 88px; height: 88px; border-radius: 50%; object-fit: cover; background: #ddd; margin-bottom: 22px; }
    label { display: block; text-align: left; font-weight: 700; font-size: 13px; margin: 12px 0 6px; }
    input { width: 100%; box-sizing: border-box; border: 1px solid #d1d5db; border-radius: 14px; padding: 12px; background: white; }
    .hint { color: #6b7280; font-size: 12px; margin-top: 12px; line-height: 1.4; }
    button { border: none; border-radius: 999px; padding: 12px 24px; font-weight: 800; cursor: pointer; background: #2b2d33; color: white; margin-top: 18px; }
    .dashboard-page { display: none; padding: 22px; max-width: 1280px; margin: 0 auto; }
    .top-bar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px; }
    .top-left { display: flex; align-items: center; gap: 14px; }
    .top-left img { width: 58px; height: 58px; border-radius: 50%; object-fit: cover; }
    .top-left h1 { margin: 0; font-size: 30px; }
    .grid { display: grid; grid-template-columns: 1.45fr 1fr; gap: 20px; }
    .card { background: rgba(255,255,255,0.92); border-radius: 24px; padding: 20px; box-shadow: 0 14px 34px rgba(0,0,0,0.08); }
    .camera-box { position: relative; background: #111; border-radius: 20px; overflow: hidden; aspect-ratio: 4 / 3; display: flex; justify-content: center; align-items: center; }
    #stream { width: 100%; height: 100%; object-fit: cover; }
    .ai-box { display: none; position: absolute; border: 3px solid #ef4444; box-shadow: 0 0 0 1px rgba(255,255,255,0.65), 0 0 18px rgba(239,68,68,0.55); box-sizing: border-box; pointer-events: none; }
    .ai-label { position: absolute; left: -3px; top: -28px; background: #ef4444; color: white; font-size: 12px; font-weight: 900; padding: 5px 8px; border-radius: 6px 6px 6px 0; white-space: nowrap; }
    .ai-corner { position: absolute; width: 13px; height: 13px; border-color: white; border-style: solid; }
    .ai-corner.tl { left: -4px; top: -4px; border-width: 2px 0 0 2px; }
    .ai-corner.tr { right: -4px; top: -4px; border-width: 2px 2px 0 0; }
    .ai-corner.bl { left: -4px; bottom: -4px; border-width: 0 0 2px 2px; }
    .ai-corner.br { right: -4px; bottom: -4px; border-width: 0 2px 2px 0; }
    canvas { width: 100%; height: 260px; }
    .metric-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; margin-top: 16px; }
    .metric { background: #fafafa; border-radius: 18px; padding: 16px; min-height: 115px; }
    .metric-title { font-size: 14px; color: #555; font-weight: 700; }
    .metric-value { font-size: 27px; font-weight: 900; margin-top: 10px; }
    .metric-note { font-size: 12px; color: #777; margin-top: 8px; line-height: 1.35; }
    .badge-good { color: #15803d; font-weight: 800; }
    .badge-warn { color: #dc2626; font-weight: 800; }
    .side-card { margin-bottom: 18px; }
    .alert { border-left: 6px solid #ef4444; background: #fff7f7; }
    .next-title { font-size: 20px; font-weight: 900; margin-bottom: 12px; }
    li { margin-bottom: 10px; line-height: 1.4; }
    .blue-btn { background: #2563eb; width: 100%; margin-top: 12px; }
    .small-guide { color: #555; line-height: 1.5; font-size: 14px; }
    .event-status { display: flex; justify-content: space-between; gap: 10px; align-items: center; margin-bottom: 12px; }
    .event-pill { border-radius: 999px; background: #111827; color: white; padding: 6px 10px; font-size: 12px; font-weight: 800; }
    .event-card { border: 1px solid #e5e7eb; border-radius: 8px; padding: 12px; margin-top: 10px; background: rgba(251,251,251,0.94); }
    .event-card.reveal { animation: cardReveal 1100ms cubic-bezier(0.16, 1, 0.3, 1), pulseGlow 1500ms ease-out; }
    .event-head { display: flex; justify-content: space-between; align-items: baseline; gap: 10px; }
    .event-name { font-weight: 900; font-size: 15px; }
    .event-time { color: #6b7280; font-size: 11px; }
    .event-score { font-size: 22px; font-weight: 900; margin: 8px 0; }
    .event-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; font-size: 12px; color: #4b5563; }
    .event-grid b { display: block; color: #111827; font-size: 15px; margin-top: 2px; }
    .event-spark { width: 100%; height: 52px; margin-top: 10px; }
    @media (max-width: 900px) { .grid { grid-template-columns: 1fr; } }
    @media (prefers-reduced-motion: reduce) { body::before, body::after, .event-card.reveal { animation: none; } }
  </style>
</head>

<body>
  <div id="loginPage" class="center-page">
    <div class="login-card">
      <h1>Add Your Cat</h1>
      <div class="subtitle">Let's begin with your cat's name and photo.</div>
      <img id="catPreview" class="cat-preview" src="https://placekitten.com/200/200">
      <label>Cat Name</label>
      <input id="catNameInput" placeholder="e.g., Luna">
      <label>Cat Photo</label>
      <input id="catPhotoInput" type="file" accept="image/*">
      <div class="hint">You can upload your own cat photo. If not, SniffKitty will use a default cat avatar.</div>
      <button onclick="saveCat()">Save Cat</button>
    </div>
  </div>

  <div id="dashboardPage" class="dashboard-page">
    <div class="top-bar">
      <div class="top-left">
        <img id="dashCatPhoto" src="https://placekitten.com/200/200">
        <div>
          <h1 id="dashTitle">SniffKitty Dashboard</h1>
          <div class="subtitle">Live camera, ammonia signal, entry frequency, and stool AI alert</div>
        </div>
      </div>
      <button onclick="logout()">Change Cat</button>
    </div>

    <div class="grid">
      <div>
        <div class="card">
          <div class="next-title">Live Cat Camera</div>
          <div class="camera-box">
            <img id="stream">
            <div class="ai-box" id="cameraAiBox">
              <div class="ai-label" id="cameraAiLabel">Change --%</div>
              <div class="ai-corner tl"></div>
              <div class="ai-corner tr"></div>
              <div class="ai-corner bl"></div>
              <div class="ai-corner br"></div>
            </div>
          </div>
        </div>

        <div class="card" style="margin-top:18px;">
          <div class="next-title">Daily Health Trend</div>
          <canvas id="trendCanvas" width="700" height="280"></canvas>
        </div>

        <div class="metric-grid">
          <div class="metric">
            <div class="metric-title">Ammonia / MQ137 Signal</div>
            <div class="metric-value" id="ammoniaLevel">--</div>
            <div class="metric-note">Baseline: MQ137 clean air level<br>Current: <span id="mqValue">--</span></div>
            <div class="metric-note badge-warn" id="ammoniaReview">Review Needed</div>
          </div>

          <div class="metric">
            <div class="metric-title">Visit Frequency</div>
            <div class="metric-value" id="entryFreq">--</div>
            <div class="metric-note">Based on camera presence<br>Current status: <span id="presenceValue">--</span></div>
            <div class="metric-note badge-good">Healthy Pattern</div>
          </div>

          <div class="metric">
            <div class="metric-title">Stay Duration</div>
            <div class="metric-value" id="stayDuration">-- sec</div>
            <div class="metric-note">If duration is too long, check litter box behavior.</div>
            <div class="metric-note badge-good">Within normal range</div>
          </div>

          <div class="metric">
            <div class="metric-title">Sulfide / MQ135 Signal</div>
            <div class="metric-value" id="airLevel">--</div>
            <div class="metric-note">Baseline: MQ135 clean air level<br>Current: <span id="mq135Value">--</span></div>
            <div class="metric-note badge-warn" id="airReview">Review Needed</div>
          </div>
        </div>
      </div>

      <div>
        <div class="card side-card alert">
          <div class="next-title">Health Summary</div>
          <p id="summaryText">Waiting for sensor data. Recent ammonia, entry, and stay duration are still within normal range.</p>
        </div>

        <div class="card side-card">
          <div class="event-status">
            <div class="next-title" style="margin-bottom:0;">Visit Signature</div>
            <div class="event-pill" id="visitLiveState">Standby</div>
          </div>
          <div class="small-guide" id="visitInsight">Waiting for the next complete litter box visit.</div>
          <div id="visitEvents"></div>
        </div>

        <div class="card side-card">
          <div class="next-title">Suggested Next Steps</div>
          <ul>
            <li>Check if the litter box needs cleaning</li>
            <li>Monitor recent food changes</li>
            <li>Keep watching stool shape and color patterns</li>
            <li>If deviation persists, consult a veterinarian</li>
          </ul>
          <button class="blue-btn">Find Vet / Online Consultation</button>
        </div>

        <div class="card side-card">
          <div class="next-title">Environment</div>
          <p>Temperature: <b id="tempValue">--</b> °C</p>
          <p>Humidity: <b id="humValue">--</b> %</p>
          <p>Ammonia Change: <b id="ammoniaChange">--</b> %</p>
          <p>Sulfide Change: <b id="airChange">--</b> %</p>
          <p>Status: <b id="statusValue">--</b></p>
          <p>Sulfide Status: <b id="airStatusValue">--</b></p>
        </div>

        <div class="card">
          <div class="next-title">How to Read This</div>
          <div class="small-guide">😊 means the signal matches your cat's baseline or trained model. ❓ means the system detected a deviation and more context is needed.</div>
        </div>
      </div>
    </div>
  </div>

<script>
  document.getElementById("stream").src = "http://" + location.hostname + ":8001/stream";

  let catPhotoData = "";
  let ammoniaHistory = [];
  let airHistory = [];
  let tempHistory = [];
  let humHistory = [];
  let activeVisit = null;
  let visitEvents = [];
  let visitSequence = 0;
  let lastPostRecording = 0;

  const photoInput = document.getElementById("catPhotoInput");
  photoInput.addEventListener("change", function() {
    const file = this.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = function(e) {
      catPhotoData = e.target.result;
      document.getElementById("catPreview").src = catPhotoData;
    };
    reader.readAsDataURL(file);
  });

  function saveCat() {
    const name = document.getElementById("catNameInput").value || "My Cat";
    const photo = catPhotoData || "https://placekitten.com/200/200";
    localStorage.setItem("sniffkitty_name", name);
    localStorage.setItem("sniffkitty_photo", photo);
    showDashboard();
  }

  function logout() {
    localStorage.removeItem("sniffkitty_name");
    localStorage.removeItem("sniffkitty_photo");
    location.reload();
  }

  function showDashboard() {
    const name = localStorage.getItem("sniffkitty_name") || "My Cat";
    const photo = localStorage.getItem("sniffkitty_photo") || "https://placekitten.com/200/200";
    document.getElementById("dashTitle").innerText = name + "'s Health Dashboard";
    document.getElementById("dashCatPhoto").src = photo;
    document.getElementById("loginPage").style.display = "none";
    document.getElementById("dashboardPage").style.display = "block";
  }

  function updateStayDurationDisplay() {
    if (window.stayStartTime === undefined || window.stayStartTime === null) {
      return;
    }

    const sec = Math.floor((Date.now() - window.stayStartTime) / 1000);
    document.getElementById("stayDuration").innerText = sec + " sec";
  }

  function updateCameraAiBox(data) {
    const box = document.getElementById("cameraAiBox");
    const label = document.getElementById("cameraAiLabel");

    if (!box || !label) return;

    const valid = Number(data.camera_box_valid) === 1;
    const change = Number(data.camera_change_percent);

    if (!valid || change < 1) {
      box.style.display = "none";
      return;
    }

    box.style.display = "block";
    box.style.left = Number(data.camera_box_x).toFixed(1) + "%";
    box.style.top = Number(data.camera_box_y).toFixed(1) + "%";
    box.style.width = Math.max(3, Number(data.camera_box_w)).toFixed(1) + "%";
    box.style.height = Math.max(3, Number(data.camera_box_h)).toFixed(1) + "%";
    label.innerText = "Change " + change.toFixed(1) + "%";
  }

  function drawChart() {
    const canvas = document.getElementById("trendCanvas");
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = "#ddd";
    ctx.lineWidth = 1;

    for (let i = 0; i <= 5; i++) {
      let y = 30 + i * 42;
      ctx.beginPath();
      ctx.moveTo(45, y);
      ctx.lineTo(680, y);
      ctx.stroke();
    }

    ctx.fillStyle = "#555";
    ctx.font = "12px Arial";
    ctx.fillText("Signal Level", 8, 25);

    function drawLine(data, color) {
      if (data.length < 2) return;

      ctx.strokeStyle = color;
      ctx.lineWidth = 3;
      ctx.beginPath();

      for (let i = 0; i < data.length; i++) {
        let x = 55 + i * (600 / Math.max(1, data.length - 1));
        let y = 240 - Math.min(200, Math.max(0, data[i] * 2));
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }

      ctx.stroke();

      for (let i = 0; i < data.length; i++) {
        let x = 55 + i * (600 / Math.max(1, data.length - 1));
        let y = 240 - Math.min(200, Math.max(0, data[i] * 2));
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, Math.PI * 2);
        ctx.fill();
      }
    }

    drawLine(ammoniaHistory, "#b45309");
    drawLine(airHistory, "#7c3aed");
    drawLine(tempHistory, "#2563eb");
    drawLine(humHistory, "#16a34a");

    ctx.fillStyle = "#2563eb";
    ctx.fillText("Temp", 70, 18);
    ctx.fillStyle = "#16a34a";
    ctx.fillText("Humidity", 130, 18);
    ctx.fillStyle = "#b45309";
    ctx.fillText("Ammonia", 220, 18);
    ctx.fillStyle = "#7c3aed";
    ctx.fillText("Sulfide", 300, 18);
  }

  function ammoniaText(percent) {
    if (percent < 10) return "Normal";
    if (percent < 30) return "Slight";
    if (percent < 80) return "Elevated";
    return "Strong";
  }

  function beginVisit(data) {
    visitSequence += 1;
    activeVisit = {
      id: visitSequence,
      start: Date.now(),
      exit: null,
      samples: [],
      ammoniaPeak: 0,
      airPeak: 0,
      cameraPeak: 0,
      tempStart: Number(data.temp_C),
      humStart: Number(data.hum_percent)
    };
    document.getElementById("visitLiveState").innerText = "Sampling";
  }

  function sampleVisit(data) {
    if (!activeVisit) return;

    const ammonia = Math.max(0, Number(data.ammonia_change_percent));
    const air = Math.max(0, Number(data.air_change_percent));
    const camera = Math.max(0, Number(data.camera_change_percent));

    activeVisit.samples.push({
      t: Date.now(),
      ammonia: ammonia,
      air: air,
      camera: camera,
      temp: Number(data.temp_C),
      hum: Number(data.hum_percent)
    });

    if (activeVisit.samples.length > 90) {
      activeVisit.samples.shift();
    }

    activeVisit.ammoniaPeak = Math.max(activeVisit.ammoniaPeak, ammonia);
    activeVisit.airPeak = Math.max(activeVisit.airPeak, air);
    activeVisit.cameraPeak = Math.max(activeVisit.cameraPeak, camera);
  }

  function markVisitExit() {
    if (activeVisit && activeVisit.exit === null) {
      activeVisit.exit = Date.now();
      document.getElementById("visitLiveState").innerText = "Recovery";
    }
  }

  function gradeVisit(event) {
    if (event.ammoniaPeak >= 80 || event.airPeak >= 80) return "Strong signature";
    if (event.ammoniaPeak >= 30 || event.airPeak >= 30) return "Elevated signature";
    return "Clean signature";
  }

  function finishVisit() {
    if (!activeVisit) return;

    if (activeVisit.samples.length < 2) {
      activeVisit = null;
      document.getElementById("visitLiveState").innerText = "Standby";
      renderVisitEvents();
      return;
    }

    const now = Date.now();
    const end = activeVisit.samples[activeVisit.samples.length - 1];
    const durationSec = Math.max(1, Math.round(((activeVisit.exit || now) - activeVisit.start) / 1000));
    const recoverySec = activeVisit.exit ? Math.max(0, Math.round((now - activeVisit.exit) / 1000)) : 0;
    const tempDelta = end.temp - activeVisit.tempStart;
    const humDelta = end.hum - activeVisit.humStart;

    const event = {
      id: activeVisit.id,
      when: new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }),
      durationSec: durationSec,
      recoverySec: recoverySec,
      ammoniaPeak: activeVisit.ammoniaPeak,
      airPeak: activeVisit.airPeak,
      cameraPeak: activeVisit.cameraPeak,
      tempDelta: tempDelta,
      humDelta: humDelta,
      samples: activeVisit.samples.slice()
    };

    event.grade = gradeVisit(event);
    visitEvents.unshift(event);
    visitEvents = visitEvents.slice(0, 3);
    activeVisit = null;
    document.getElementById("visitLiveState").innerText = "Standby";
    renderVisitEvents(event.id);
  }

  function drawEventSparkline(canvas, samples) {
    const ctx = canvas.getContext("2d");
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);
    ctx.strokeStyle = "#e5e7eb";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, h - 8);
    ctx.lineTo(w, h - 8);
    ctx.stroke();

    if (samples.length < 2) return;

    function line(key, color) {
      ctx.strokeStyle = color;
      ctx.lineWidth = 3;
      ctx.beginPath();

      for (let i = 0; i < samples.length; i++) {
        const x = i * (w / Math.max(1, samples.length - 1));
        const y = h - 8 - Math.min(h - 12, Math.max(0, samples[i][key] * 1.2));
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }

      ctx.stroke();
    }

    line("ammonia", "#b45309");
    line("air", "#7c3aed");
  }

  function renderVisitEvents(revealEventId) {
    const container = document.getElementById("visitEvents");

    if (visitEvents.length === 0) {
      container.innerHTML = "";
      document.getElementById("visitInsight").innerText =
        activeVisit ? "Building a gas signature from this visit." : "Waiting for the next complete litter box visit.";
      return;
    }

    document.getElementById("visitInsight").innerText =
      visitEvents[0].grade + " captured from the latest visit.";

    container.innerHTML = visitEvents.map(function(event, index) {
      const revealClass = event.id === revealEventId ? "reveal" : "";
      return `
        <div class="event-card ${revealClass}">
          <div class="event-head">
            <div class="event-name">Visit #${event.id}</div>
            <div class="event-time">${event.when}</div>
          </div>
          <div class="event-score">${event.grade}</div>
          <div class="event-grid">
            <div>Duration<b>${event.durationSec}s</b></div>
            <div>Recovery<b>${event.recoverySec}s</b></div>
            <div>NH3 Peak<b>+${event.ammoniaPeak.toFixed(1)}%</b></div>
            <div>Sulfide Peak<b>+${event.airPeak.toFixed(1)}%</b></div>
            <div>Camera Peak<b>${event.cameraPeak.toFixed(1)}%</b></div>
            <div>Env Drift<b>${event.tempDelta.toFixed(1)}C / ${event.humDelta.toFixed(1)}%</b></div>
          </div>
          <canvas class="event-spark" id="eventSpark${index}" width="310" height="52"></canvas>
        </div>
      `;
    }).join("");

    visitEvents.forEach(function(event, index) {
      drawEventSparkline(document.getElementById("eventSpark" + index), event.samples);
    });
  }

async function updateData() {
  try {
    const response = await fetch("/data");
    const data = await response.json();

    document.getElementById("tempValue").innerText =
      Number(data.temp_C).toFixed(2);

    document.getElementById("humValue").innerText =
      Number(data.hum_percent).toFixed(2);

    document.getElementById("mqValue").innerText =
      data.mq137_raw;

    document.getElementById("mq135Value").innerText =
      data.mq135_raw;

    document.getElementById("ammoniaChange").innerText =
      Number(data.ammonia_change_percent).toFixed(1);

    document.getElementById("airChange").innerText =
      Number(data.air_change_percent).toFixed(1);

    document.getElementById("statusValue").innerText =
      data.status;

    document.getElementById("airStatusValue").innerText =
      data.air_status;

    document.getElementById("ammoniaLevel").innerText =
      ammoniaText(Number(data.ammonia_change_percent));

    document.getElementById("airLevel").innerText =
      ammoniaText(Number(data.air_change_percent));

    updateCameraAiBox(data);

    // Ammonia status
    if (Number(data.ammonia_change_percent) < 30) {

      document.getElementById("ammoniaReview").innerText =
        "Normal Pattern";

      document.getElementById("ammoniaReview").className =
        "metric-note badge-good";

    } else {

      document.getElementById("ammoniaReview").innerText =
        "Review Needed";

      document.getElementById("ammoniaReview").className =
        "metric-note badge-warn";
    }

    if (Number(data.air_change_percent) < 30) {

      document.getElementById("airReview").innerText =
        "Normal Pattern";

      document.getElementById("airReview").className =
        "metric-note badge-good";

    } else {

      document.getElementById("airReview").innerText =
        "Review Needed";

      document.getElementById("airReview").className =
        "metric-note badge-warn";
    }

    // 初始化变量
    if (window.entryCount === undefined)
      window.entryCount = 0;

    if (window.lastPresenceState === undefined)
      window.lastPresenceState = 0;

    if (window.stayStartTime === undefined)
      window.stayStartTime = null;

    if (window.entryCountedThisVisit === undefined)
      window.entryCountedThisVisit = false;

    let presentNow = Number(data.present);
    let recordingNow = Number(data.recording);
    let postRecordingNow = Number(data.post_recording);
    const entryMinDurationMs = 5000;

    // Presence 0 -> 1
    if (presentNow === 1 && window.lastPresenceState === 0) {


      // 开始时间
      window.stayStartTime = Date.now();
      window.entryCountedThisVisit = false;

      // 一开始先显示 0 sec
      document.getElementById("stayDuration").innerText =
        "0 sec";

      beginVisit(data);
    }

    // Presence stays 1
    if (presentNow === 1) {

      updateStayDurationDisplay();

      if (
        !window.entryCountedThisVisit &&
        window.stayStartTime !== null &&
        Date.now() - window.stayStartTime >= entryMinDurationMs
      ) {
        window.entryCount += 1;
        window.entryCountedThisVisit = true;
      }

      document.getElementById("entryFreq").innerText =
        window.entryCount + "/day";

      document.getElementById("presenceValue").innerText =
        "Present";
    }

    // Presence = 0
    if (presentNow === 0) {

      if (window.lastPresenceState === 1) {
        markVisitExit();
      }

      // 立刻清零
      window.stayStartTime = null;
      window.entryCountedThisVisit = false;

      document.getElementById("stayDuration").innerText =
        "0 sec";

      document.getElementById("entryFreq").innerText =
        window.entryCount + "/day";

      document.getElementById("presenceValue").innerText =
        "Empty";
    }

    // 更新上一状态
    if (activeVisit && (presentNow === 1 || recordingNow === 1 || postRecordingNow === 1)) {
      sampleVisit(data);
    }

    if (
      activeVisit &&
      activeVisit.exit !== null &&
      ((lastPostRecording === 1 && postRecordingNow === 0) || recordingNow === 0)
    ) {
      finishVisit();
    } else if (activeVisit) {
      renderVisitEvents();
    }

    window.lastPresenceState = presentNow;
    lastPostRecording = postRecordingNow;

    // Summary
    if (Number(data.ammonia_change_percent) >= 30) {

      document.getElementById("summaryText").innerText =
        "Ammonia signal is higher than baseline. Check litter box condition and keep monitoring recent behavior.";

    } else {

      document.getElementById("summaryText").innerText =
        "Recent ammonia, entry frequency, and stay duration are still within normal range.";
    }

    // 图表
    ammoniaHistory.push(
      Math.max(0, Number(data.ammonia_change_percent))
    );

    airHistory.push(
      Math.max(0, Number(data.air_change_percent))
    );

    tempHistory.push(
      Number(data.temp_C)
    );

    humHistory.push(
      Number(data.hum_percent)
    );

    if (ammoniaHistory.length > 8) {
      ammoniaHistory.shift();
      airHistory.shift();
      tempHistory.shift();
      humHistory.shift();
    }

    drawChart();

  } catch (err) {

    console.log("Data update failed", err);

  }
}

  if (localStorage.getItem("sniffkitty_name")) {
    showDashboard();
  }

  setInterval(updateData, 1000);
  setInterval(updateStayDurationDisplay, 200);
  updateData();
</script>

</body>
</html>
)rawliteral";

#endif
