constexpr char html_main[] = R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Real-time Chart</title>
  <!-- D3.js -->
  <script src="https://d3js.org/d3.v6.min.js"></script>
  <!-- WebSocket library -->
  <script src="https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.0.1/socket.io.js"></script>
</head>
<body>
<h1>ESP32 Web Server</h1>
<!-- Chart container -->
<div id="chart-container" style="width: 80%; margin: auto;"></div>

<script>
  // Connect to WebSocket server
  const socket = io.connect('your_websocket_server_url');

  // Initialize chart
  const margin = { top: 20, right: 20, bottom: 30, left: 50 };
  const width = 800 - margin.left - margin.right;
  const height = 400 - margin.top - margin.bottom;

  const svg = d3.select("#chart-container")
    .append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", `translate(${margin.left},${margin.top})`);

  const xScale = d3.scaleLinear().range([0, width]);
  const yScale = d3.scaleLinear().range([height, 0]);

  const line = d3.line()
    .x((d, i) => xScale(i))
    .y(d => yScale(d));

  const path = svg.append("path");

  // Handle incoming data from WebSocket
  socket.on('data', data => {
    // Update chart with new data
    updateChart(data);
  });

  // Function to update the chart
  function updateChart(data) {
    xScale.domain([0, data.length - 1]);
    yScale.domain([0, d3.max(data)]);

    path.data([data])
      .attr("d", line)
      .attr("fill", "none")
      .attr("stroke", "steelblue");
  }
</script>

</body>
</html>
)";
