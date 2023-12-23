function updateTime() {
  const now = new Date();
  const hours = now.getHours().toString().padStart(2, '0');
  const minutes = now.getMinutes().toString().padStart(2, '0');
  const seconds = now.getSeconds().toString().padStart(2, '0');
  const formattedTime = `${hours}:${minutes}:${seconds}`;

  document.getElementById('time').innerText = formattedTime;
}

const button = document.getElementById('waterButton');
button.addEventListener('click', function(e){
    console.log('button was clicked');

    fetch('/clicked', {method: 'POST'})
        .then(function(response){
            if(response.ok){
                console.log('Click was recorded');
                return;
            }
            throw new Error('Request failed.');
        })
        .catch(function(error){
            console.log(error);
        });
});

function readSoilMoisture() {
  const url = 'soilMoisture.txt';
  const cacheBuster = Date.now(); // Add a timestamp to the URL to bust the cache
  const urlWithCacheBuster = `${url}?${cacheBuster}`;

  fetch(urlWithCacheBuster)
    .then(response => response.text())
    .then(text => {
      // Assuming text contains the soil moisture value
      const soilMoistureDataElement = document.getElementById('soilMoistureData');
      if (soilMoistureDataElement) {
        soilMoistureDataElement.innerText = text + '%';
      }
    })
    .catch(error => console.error('Error fetching soil moisture:', error));
}

// Call readFromFileAndUpdate every second
setInterval(readSoilMoisture, 1000);

function readTemperature() {
  const url = 'temperature.txt';
  const cacheBuster = Date.now(); // Add a timestamp to the URL to bust the cache
  const urlWithCacheBuster = `${url}?${cacheBuster}`;

  fetch(urlWithCacheBuster)
    .then(response => response.text())
    .then(text => {
      // Assuming text contains the soil moisture value
      const tempDataElement = document.getElementById('tempData');
      if (tempDataElement) {
        tempDataElement.innerText = text + '°C';
      }
    })
    .catch(error => console.error('Error fetching temperature:', error));
}

// Call readFromFileAndUpdate every second
setInterval(readTemperature, 1000);

function readLastTime() {
  const url = 'timeSinceWater.txt';
  const cacheBuster = Date.now(); // Add a timestamp to the URL to bust the cache
  const urlWithCacheBuster = `${url}?${cacheBuster}`;

  fetch(urlWithCacheBuster)
    .then(response => response.text())
    .then(text => {
      // Assuming text contains the soil moisture value
      const lastTimeElement = document.getElementById('lastWater');
      if (lastTimeElement) {
        lastTimeElement.innerText = text;
      }
    })
    .catch(error => console.error('Error fetching last time moisture:', error));
}

// Call readFromFileAndUpdate every second
setInterval(readLastTime, 1000);

// Update time every second
setInterval(updateTime, 1000);

// Initial update
updateTime();