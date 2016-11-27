var sio = require("socket.io-client");

Pebble.addEventListener('ready', function() {
  // PebbleKit JS is ready!
  console.log('PebbleKit JS ready!');
  
    // Update s_js_ready on watch
  Pebble.sendAppMessage({'JSReady': 1});
  
  Pebble.sendAppMessage({'Target': 7});
});

var act_done = false;

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) { 
    var dict = e.payload;
    
    if(dict['ActivityDone'] === true) {
      act_done = true;
    }
    
    console.log('AppMessage received - ActivityDone =  ' + dict['ActivityDone']);
    
    var socket = sio('https://young-mesa-30289.herokuapp.com/');
    socket.send('DONE');
    
    console.log('Sent DONE signal to server');
  }                     
);