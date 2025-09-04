/* eslint-disable max-classes-per-file */
/* eslint-disable no-restricted-globals */
/* eslint-disable no-undef */
$(document).ready(() => {
  // if deployed to a site supporting SSL, use wss://
  const protocol = document.location.protocol.startsWith('https') ? 'wss://' : 'ws://';
  const webSocket = new WebSocket(protocol + location.host);

  // A class for holding the last N points of telemetry for a device
  class DeviceData {
    constructor(deviceId) {
      this.deviceId = deviceId;
      this.maxLen = 50;
        this.timeData       = [];
        this.temperatureData = [];
        this.humidityData   = [];
        this.co2Data  = [];
        this.eco2Data = [];
        this.tvocData = [];
        this.lightRawData = [];
        this.lightPctData = [];

    }

   addData(time, temperature, humidity, co2, eco2, tvoc, light_raw, light_pct) {
  const pushOrNull = (arr, v) => arr.push((typeof v === 'number') ? v : null);
  this.timeData.push(time);
  pushOrNull(this.temperatureData, temperature);
  pushOrNull(this.humidityData, humidity);
  pushOrNull(this.co2Data, co2);
  pushOrNull(this.eco2Data, eco2);
  pushOrNull(this.tvocData, tvoc);
  pushOrNull(this.lightRawData, light_raw);
  pushOrNull(this.lightPctData, light_pct);
  const trim = (arr) => { if (arr.length > this.maxLen) arr.shift(); };
  [this.timeData, this.temperatureData, this.humidityData,
   this.co2Data, this.eco2Data, this.tvocData, this.lightRawData, this.lightPctData].forEach(trim);
}

    }
  

  // All the devices in the list (those that have been sending telemetry)
  class TrackedDevices {
    constructor() {
      this.devices = [];
    }

    // Find a device based on its Id
    findDevice(deviceId) {
      for (let i = 0; i < this.devices.length; ++i) {
        if (this.devices[i].deviceId === deviceId) {
          return this.devices[i];
        }
      }

      return undefined;
    }

    getDevicesCount() {
      return this.devices.length;
    }
  }

  const trackedDevices = new TrackedDevices();

  // Define the chart axes
  const chartData = {
  datasets: [
    {
      fill: false,
      label: 'Temperature',
      yAxisID: 'Temperature',
      borderColor: 'rgba(255, 204, 0, 1)',
      pointBoarderColor: 'rgba(255, 204, 0, 1)',
      backgroundColor: 'rgba(255, 204, 0, 0.4)',
      pointHoverBackgroundColor: 'rgba(255, 204, 0, 1)',
      pointHoverBorderColor: 'rgba(255, 204, 0, 1)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'Humidity',
      yAxisID: 'Humidity',
      borderColor: 'rgba(24, 120, 240, 1)',
      pointBoarderColor: 'rgba(24, 120, 240, 1)',
      backgroundColor: 'rgba(24, 120, 240, 0.4)',
      pointHoverBackgroundColor: 'rgba(24, 120, 240, 1)',
      pointHoverBorderColor: 'rgba(24, 120, 240, 1)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'CO2',
      yAxisID: 'Gas',
      borderColor: 'rgba(220, 20, 60, 1)',
      backgroundColor: 'rgba(220, 20, 60, 0.3)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'eCO2',
      yAxisID: 'Gas',
      borderColor: 'rgba(0, 200, 140, 1)',
      backgroundColor: 'rgba(0, 200, 140, 0.3)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'TVOC',
      yAxisID: 'Gas',
      borderColor: 'rgba(140, 0, 200, 1)',
      backgroundColor: 'rgba(140, 0, 200, 0.3)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'Light Raw',
      yAxisID: 'Temperature',
      borderColor: 'rgba(100, 100, 100, 1)',
      backgroundColor: 'rgba(100, 100, 100, 0.3)',
      spanGaps: true,
    },
    {
      fill: false,
      label: 'Light %',
      yAxisID: 'Humidity', 
      borderColor: 'rgba(255, 99, 132, 1)',
      backgroundColor: 'rgba(255, 99, 132, 0.3)',
      spanGaps: true,
    },
  ]
};

const chartOptions = {
  scales: {
    yAxes: [
      {
        id: 'Temperature',
        type: 'linear',
        scaleLabel: { labelString: 'Temperature / Light (ºC / raw)', display: true },
        position: 'left',
        ticks: { beginAtZero: true }
      },
      {
        id: 'Humidity',
        type: 'linear',
        scaleLabel: { labelString: 'Humidity / Light% (%)', display: true },
        position: 'right',
        ticks: { beginAtZero: true }
      },
      {
        id: 'Gas',
        type: 'linear',
        scaleLabel: { labelString: 'Gas (ppm / ppb)', display: true },
        position: 'right',
        gridLines: { drawOnChartArea: false },
        ticks: { beginAtZero: true }
      }
    ]
  }
};


  // Get the context of the canvas element we want to select
  const ctx = document.getElementById('iotChart').getContext('2d');
  const myLineChart = new Chart(
    ctx,
    {
      type: 'line',
      data: chartData,
      options: chartOptions,
    });

  // Manage a list of devices in the UI, and update which device data the chart is showing
  // based on selection
  let needsAutoSelect = true;
  const deviceCount = document.getElementById('deviceCount');
  const listOfDevices = document.getElementById('listOfDevices');
  function OnSelectionChange() {
  const device = trackedDevices.findDevice(listOfDevices[listOfDevices.selectedIndex].text);
  if (!device) return;
  chartData.labels           = device.timeData;
  chartData.datasets[0].data = device.temperatureData;
  chartData.datasets[1].data = device.humidityData;
  chartData.datasets[2].data = device.co2Data;
  chartData.datasets[3].data = device.eco2Data;
  chartData.datasets[4].data = device.tvocData;
  chartData.datasets[5].data = device.lightRawData;
  chartData.datasets[6].data = device.lightPctData;
  myLineChart.update();
}

  listOfDevices.addEventListener('change', OnSelectionChange, false);

  // When a web socket message arrives:
  // 1. Unpack it
  // 2. Validate it has date/time and temperature
  // 3. Find or create a cached device to hold the telemetry data
  // 4. Append the telemetry data
  // 5. Update the chart UI
  webSocket.onmessage = function onMessage(message) {
    try {
      const messageData = JSON.parse(message.data);
      console.log(messageData);


      const hasAny =
        ['temperature','humidity','co2','eco2','tvoc','light_raw','light_pct']
          .some(k => typeof (messageData.IotData || {})[k] === 'number');
      if (!messageData.MessageDate || !hasAny) {
        return;
      }


      // find or add device to list of tracked devices
      const existingDeviceData = trackedDevices.findDevice(messageData.DeviceId);

      if (existingDeviceData) {
        existingDeviceData.addData(        
        messageData.MessageDate,
        messageData.IotData.temperature,
        messageData.IotData.humidity,
        messageData.IotData.co2,
        messageData.IotData.eco2,
        messageData.IotData.tvoc,
        messageData.IotData.light_raw,
        messageData.IotData.light_pct);
        const selectedText = listOfDevices[listOfDevices.selectedIndex]?.text;
  if (selectedText === messageData.DeviceId) {
    OnSelectionChange();
  }
      } else {
        const newDeviceData = new DeviceData(messageData.DeviceId);
        trackedDevices.devices.push(newDeviceData);
        const numDevices = trackedDevices.getDevicesCount();
        deviceCount.innerText = numDevices === 1 ? `${numDevices} device` : `${numDevices} devices`;
          newDeviceData.addData(
        messageData.MessageDate,
        messageData.IotData.temperature,
        messageData.IotData.humidity,
        messageData.IotData.co2,
        messageData.IotData.eco2,
        messageData.IotData.tvoc,
        messageData.IotData.light_raw,
        messageData.IotData.light_pct
      );

        // add device to the UI list
        const node = document.createElement('option');
        const nodeText = document.createTextNode(messageData.DeviceId);
        node.appendChild(nodeText);
        listOfDevices.appendChild(node);

        // if this is the first device being discovered, auto-select it
        if (needsAutoSelect) {
          needsAutoSelect = false;
          listOfDevices.selectedIndex = 0;
          OnSelectionChange();
        }
      }

      myLineChart.update();
    } catch (err) {
      console.error(err);
    }
  };
});
