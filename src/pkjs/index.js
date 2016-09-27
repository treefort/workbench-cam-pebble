var SERVICE_ENDPOINT = 'http://[server hostname or IP]:4444/cam/';

Pebble.addEventListener('ready', function(e) {
  // ready to communicate with the watchapp
  // we aren't using this because there's nothing to 'sync'
  // when we first load the watchapp
});

Pebble.addEventListener('appmessage', function(e) {
  // we've received a message from the watchapp
  var msg = e.payload;
  if (msg.CamAction == '1') {
    runCamAction('start');
  } else {
    runCamAction('stop');
  }
});

function runCamAction(action) {
  var method = 'GET';
  var url = SERVICE_ENDPOINT + action

  // Create the request
  var request = new XMLHttpRequest();

  request.onreadystatechange = function () {
    if(request.readyState === XMLHttpRequest.DONE) {
      if (request.status === 200) {
        // things went well, tell the watchapp
        Pebble.sendAppMessage({'CamActionResult': 1});
      } else if (request.status === 0) {
        // things went not so well, tell the watchapp
        Pebble.sendAppMessage({'CamActionResult': 0});
      }
    }
  };

  // Send the request
  request.open(method, url);
  request.send();
}
