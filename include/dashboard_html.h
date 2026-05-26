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
    .camera-box { background: #111; border-radius: 20px; overflow: hidden; height: 360px; display: flex; justify-content: center; align-items: center; }
    #stream { width: 100%; height: 100%; object-fit: cover; }
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
    @media (max-width: 900px) { .grid { grid-template-columns: 1fr; } }
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
          <div class="camera-box"><img id="stream"></div>
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
            <div class="metric-title">Camera Stool AI</div>
            <div class="metric-value">Shape Alert</div>
            <div class="metric-note">Monitor stool shape from camera manually.</div>
            <div class="metric-note badge-warn">Visual Review</div>
          </div>
        </div>
      </div>

      <div>
        <div class="card side-card alert">
          <div class="next-title">Health Summary</div>
          <p id="summaryText">Waiting for sensor data. Recent ammonia, entry, and stay duration are still within normal range.</p>
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
          <p>Status: <b id="statusValue">--</b></p>
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
  let tempHistory = [];
  let humHistory = [];

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
    drawLine(tempHistory, "#2563eb");
    drawLine(humHistory, "#16a34a");

    ctx.fillStyle = "#2563eb";
    ctx.fillText("Temp", 70, 18);
    ctx.fillStyle = "#16a34a";
    ctx.fillText("Humidity", 130, 18);
    ctx.fillStyle = "#b45309";
    ctx.fillText("Ammonia", 220, 18);
  }

  function ammoniaText(percent) {
    if (percent < 10) return "Normal";
    if (percent < 30) return "Slight";
    if (percent < 80) return "Elevated";
    return "Strong";
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

    document.getElementById("ammoniaChange").innerText =
      Number(data.ammonia_change_percent).toFixed(1);

    document.getElementById("statusValue").innerText =
      data.status;

    document.getElementById("ammoniaLevel").innerText =
      ammoniaText(Number(data.ammonia_change_percent));

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
    const entryMinDurationMs = 5000;

    // Presence 0 -> 1
    if (presentNow === 1 && window.lastPresenceState === 0) {


      // 开始时间
      window.stayStartTime = Date.now();
      window.entryCountedThisVisit = false;

      // 一开始先显示 0 sec
      document.getElementById("stayDuration").innerText =
        "0 sec";
    }

    // Presence stays 1
    if (presentNow === 1) {

      let sec = 0;

      if (window.stayStartTime !== null) {
        sec = Math.floor(
          (Date.now() - window.stayStartTime) / 1000
        );
      }

      document.getElementById("stayDuration").innerText =
        sec + " sec";

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
    window.lastPresenceState = presentNow;

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

    tempHistory.push(
      Number(data.temp_C)
    );

    humHistory.push(
      Number(data.hum_percent)
    );

    if (ammoniaHistory.length > 8) {
      ammoniaHistory.shift();
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
  updateData();
</script>

</body>
</html>
)rawliteral";

#endif
