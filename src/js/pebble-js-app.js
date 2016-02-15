var SERVICE_OPEN_WEATHER   = "open";
var SERVICE_WUNDER_WEATHER = "wunder";
var EXTERNAL_DEBUG_URL     = '';
var CONFIGURATION_URL      = 'http://jaredbiehler.github.io/weather-my-way/config/';

var Global = {
    externalDebug:     false, // POST logs to external server - dangerous! lat lon recorded
    wuApiKey:          null, // register for a free api key!
    openWeatherApiKey: 'ecbc09f26372154da892444daf8fba4a',
    hourlyIndex1:      2, // 3 Hours from now
    hourlyIndex2:      8, // 9 hours from now
    updateInProgress:  false,
    updateWaitTimeout: 1 * 60 * 1000, // one minute in ms
    lastUpdateAttempt: new Date(),
    maxRetry:          3,
    retryWait:         500, // ms
    config: {
        debugEnabled:   true,
        batteryEnabled: true,
        weatherService: SERVICE_WUNDER_WEATHER,
        weatherScale:   'F'
  }
};

// Allow console messages to be turned on / off 
(function(){
    var original = console.log;
    var logMessage = function (message) {
        if (Global.config.debugEnabled) {
          original.apply(console, arguments);
        }  
    };
    console.log  = logMessage;
    console.warn = logMessage;
})();


// Setup Pebble Event Listeners
Pebble.addEventListener("ready", function(e) {
    console.log("Starting ...");
    var data = { "js_ready": true };
    Pebble.sendAppMessage(data, ack, function(e){
      nack(data);
    });
    var initialInstall = window.localStorage.getItem('initialInstall');
    if (initialInstall === null && Global.wuApiKey === null) {
      window.localStorage.setItem('initialInstall', false);
      Pebble.showSimpleNotificationOnPebble("API Key Needed", "This watchface requires a free API key from Weather Underground. Please visit Settings in the Pebble App to find out more!");
    }
});

Pebble.addEventListener("appmessage", function(data) {
    console.log("Got a message - Starting weather request ... " + JSON.stringify(data));
    try {
      Global.config.weatherService = data.payload.service === SERVICE_OPEN_WEATHER ? SERVICE_OPEN_WEATHER : SERVICE_WUNDER_WEATHER;
      Global.config.debugEnabled   = data.payload.debug   === 1;
      Global.config.batteryEnabled = data.payload.battery === 1;
      Global.config.weatherScale   = data.payload.scale   === 'C' ? 'C' : 'F';
      Global.wuApiKey              = window.localStorage.getItem('wuApiKey');
      updateWeather();
    } catch (ex) {
      console.warn("Could not retrieve data sent from Pebble: "+ex.message);
    }
});

/**
 * This is the reason for the Global.config variable - I couldn't think of a way (short of asking Pebble)
 * for the latest config settings. So I persist them in a rather ugly Global variable. 
 */
Pebble.addEventListener("showConfiguration", function (e) {
    var options = {
      's': Global.config.weatherService,
      'd': Global.config.debugEnabled,
      'u': Global.config.weatherScale,
      'b': Global.config.batteryEnabled ? 'on' : 'off',
      'a': Global.wuApiKey
    };
    var url = CONFIGURATION_URL+'?'+serialize(options);
    console.log('Configuration requested using url: '+url);
    Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
    /*
     * Android Hack - for whatever reason this event is always firing on Android with a message of 'CANCELLED'
     */
    if (e.response && e.response !== 'CANCELLED') {
      try {
        var settings = JSON.parse(decodeURIComponent(e.response));

        // Android 'cancel' sends a blank object
        if (Object.keys(settings).length <= 0) {
          return; 
        }

        console.log("Settings received: "+JSON.stringify(settings));

        var refreshNeeded = (settings.service  !== Global.config.weatherService ||
                             settings.scale    !== Global.config.weatherScale   || 
                             settings.wuApiKey !== Global.wuApiKey);

        Global.config.weatherService = settings.service === SERVICE_OPEN_WEATHER ? SERVICE_OPEN_WEATHER : SERVICE_WUNDER_WEATHER;
        Global.config.weatherScale   = settings.scale   === 'C' ? 'C' : 'F';
        Global.config.debugEnabled   = settings.debug   === 'true';
        Global.config.batteryEnabled = settings.battery === 'on';
        Global.wuApiKey              = settings.wuApiKey;

        if (Global.wuApiKey !== null) {
          window.localStorage.setItem('wuApiKey', Global.wuApiKey);
        } else {
          window.localStorage.removeItem('wuApiKey');
        }
        
        var config = {
          service: Global.config.weatherService,
          scale:   Global.config.weatherScale,
          debug:   Global.config.debugEnabled   ? 1 : 0,
          battery: Global.config.batteryEnabled ? 1 : 0
        };

        Pebble.sendAppMessage(config, ack, function(ev){
          nack(config);
        });

        if (refreshNeeded) {
          updateWeather();
        }
      } catch(ex) {
        console.warn("Unable to parse response from configuration:"+ex.message);
      }
    }
});

var ack  = function ()  { console.log("Pebble ACK sendAppMessage");}
var nack = function (data, retry) {
  retry = typeof retry !== 'undefined' ? retry : 0;
  retry++;
  if (retry >= Global.maxRetry) {
    console.warn("Pebble NACK sendAppMessage max exceeded");
    return;
  } 
  console.warn("Pebble NACK sendAppMessage retryCount:"+retry+" data:"+JSON.stringify(data));
  if (data) {
    setTimeout(function(){
          Pebble.sendAppMessage(data, ack, function(e){
            nack(data, retry);
          });
    }, Global.retryWait + Math.floor(Math.random() * Global.retryWait));
  }
};

var updateWeather = function () {
  if (Global.updateInProgress && 
      new Date().getTime() < Global.lastUpdateAttempt.getTime() + Global.updateWaitTimeout) {
    console.log("Update already started in the last "+(Global.updateWaitTimeout/60000)+" minutes");
    return;
  }
  Global.updateInProgress  = true;
  Global.lastUpdateAttempt = new Date();

  var locationOptions = { "timeout": 15000, "maximumAge": 60000 };
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
};

var locationSuccess = function (pos) {
    var coordinates = pos.coords;
    console.log("Got coordinates: " + JSON.stringify(coordinates));
    if (Global.config.weatherService === SERVICE_OPEN_WEATHER) {
      fetchOpenWeather(coordinates.latitude, coordinates.longitude);
    } else {
      fetchWunderWeatherCurrent(coordinates.latitude, coordinates.longitude);
    }
    if (Global.wuApiKey !== null) {
      fetchWunderWeatherHourly(coordinates.latitude, coordinates.longitude);
    } else {
      var data = {hourly_enabled: 0};
      console.log("Hourly disabled, no WU ApiKey");
      Pebble.sendAppMessage(data, ack, function(ev){
          nack(data);
      });
    }
};

var locationError = function (err) {
    var message = 'Location error (' + err.code + '): ' + err.message;
    console.warn(message);
    Pebble.sendAppMessage({ "error": "Loc unavailable" }, ack, nack);
    postDebugMessage({"error": message});
    Global.updateInProgress = false;
};

var fetchOpenWeather = function(latitude, longitude) {

  var options = {};
  options.url = "http://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&cnt=1&appid=" + Global.openWeatherApiKey;

  options.parse = function(response) {
      var temperature, sunrise, sunset, condition, pubdate;

      var tempResult = response.main.temp;
      if (Global.config.weatherScale === 'C') {
        // Convert temperature to Celsius
        temperature = Math.round(tempResult - 273.15);
      } 
      else {
        // Otherwise, convert temperature to Fahrenheit 
        temperature = Math.round(((tempResult - 273.15) * 1.8) + 32);
      }

      condition = response.weather[0].id;
      sunrise   = response.sys.sunrise;
      sunset    = response.sys.sunset;
      pubdate   = new Date(response.dt * 1000);

      return {
        condition:   condition,
        temperature: temperature, 
        sunrise:     sunrise,
        sunset:      sunset,
        locale:      response.name,
        pubdate:     pubdate.getHours()+':'+('0'+pubdate.getMinutes()).slice(-2),
        tzoffset:    0 // new Date().getTimezoneOffset() * 60
      };
  };
  fetchWeather(options);
};

var fetchWunderWeatherCurrent = function (latitude, longitude) {

    var options = {};
    options.url = 'http://api.wunderground.com/api/' + Global.wuApiKey +
        '/conditions/astronomy/q/'+latitude+','+longitude+'.json';

    /*
     options.parse = function(response) {
         var sunrise, sunset, pubdate, locale;
         sunrise = response.query.results.results[0].channel.astronomy.sunrise;
         sunset  = response.query.results.results[0].channel.astronomy.sunset;
         pubdate = new Date(Date.parse(response.query.results.results[0].channel.item.pubDate));
         locale  = response.query.results.results[1].Result.neighborhood;

         if (locale === null) {
            locale = response.query.results.results[1].Result.city;
         }
         if (locale === null) {
            locale = 'unknown';
         }

     return {
         condition:   parseInt(response.query.results.results[0].channel.item.condition.code),
         temperature: parseInt(response.query.results.results[0].channel.item.condition.temp),
         sunrise:     Date.parse(new Date().toDateString()+" "+sunrise) / 1000,
         sunset:      Date.parse(new Date().toDateString()+" "+sunset) / 1000,
         locale:      locale,
         pubdate:     pubdate.getHours()+':'+('0'+pubdate.getMinutes()).slice(-2),
         tzoffset:    new Date().getTimezoneOffset() * 60
     };
     */

    options.parse = function(response) {

        var co = response.current_observation;
        var pubdate   = new Date(co.observation_epoch * 1000);

        var sp =  response.sun_phase;
        var sunrise = sp.sunrise.hour + ":" + sp.sunrise.minute;  // 6:59
        var sunset  = sp.sunset.hour + ":" + sp.sunset.minute;  // 17:30

        return {
            condition:   translateConditionToFCTCode(co.weather),
            temperature: Global.config.weatherScale === 'C' ? Math.round(co.temp_c) : Math.round(co.temp_f),
            sunrise:     Date.parse(new Date().toDateString()+" "+sunrise) / 1000,
            sunset:      Date.parse(new Date().toDateString()+" "+sunset) / 1000,
            locale:      co.observation_location.city,
            pubdate:     pubdate.getHours()+':'+('0'+pubdate.getMinutes()).slice(-2),
            tzoffset:    0 // new Date().getTimezoneOffset() * 60
        };
    };
    fetchWeather(options);
};

var fetchWunderWeatherHourly = function(latitude, longitude) {

  var options = {};
  options.url = 'http://api.wunderground.com/api/'+Global.wuApiKey+'/hourly/q/'+latitude+','+longitude+'.json';

  options.parse = function(response) {
        
      var h1 = response.hourly_forecast[Global.hourlyIndex1]
        , h2 = response.hourly_forecast[Global.hourlyIndex2];  

      return {
        h1_temp: Global.config.weatherScale === 'C' ? Math.round(h1.temp.metric) : Math.round(h1.temp.english),
        h1_cond: parseInt(h1.fctcode), 
        h1_time: parseInt(h1.FCTTIME.epoch),
        h1_pop:  parseInt(h1.pop),
        h2_temp: Global.config.weatherScale === 'C' ? Math.round(h2.temp.metric) : Math.round(h2.temp.english),
        h2_cond: parseInt(h2.fctcode), 
        h2_time: parseInt(h2.FCTTIME.epoch),
        h2_pop:  parseInt(h2.pop)
      };
  };
  fetchWeather(options);
};


var fetchWeather = function(options) {

  console.log('URL: ' + options.url);

  getJson(options.url, function(err, response) {

    try {
      if (err) {
        throw err;
      }

      var weather = options.parse(response);

      console.log('Weather Data: ' + JSON.stringify(weather));

      Pebble.sendAppMessage(weather, ack, function(e){
        nack(weather);
      });
      postDebugMessage(weather);
    
    } catch (ex) {
      console.warn("Could not find weather data in response: " + ex.message);
      var error = { "error": "HTTP Error" };
      Pebble.sendAppMessage(error, ack, nack);
      postDebugMessage(error);
    }
    Global.updateInProgress = false;
  });
};

var getJson = function(url, callback) {
  try {
    var req = new XMLHttpRequest();
    req.open('GET', url, true);
    req.onload = function(e) {
      if (req.readyState == 4) {
        if(req.status == 200) {
          try {
            //console.log(req.responseText);
            var response = JSON.parse(req.responseText);
            callback(null, response);
          } catch (ex) {
            callback(ex.message);
          }
        } else {
          callback("Error request status not 200, status: "+req.status);
        }
      }
    };
    req.send(null);
  } catch(ex) {
    callback("Unable to GET JSON: "+ex.message);
  }
};

var postDebugMessage = function (data) {
  if (!Global.externalDebug || EXTERNAL_DEBUG_URL === null) {
    return;
  }
  try {
    post(EXTERNAL_DEBUG_URL, 'data='+JSON.stringify(data));
  } catch (ex) {
    console.warn('Post Debug Message failed:'+ex.message);
  }  
};

var post = function(url, data) {
  try {
    var req = new XMLHttpRequest();
    req.open('POST', url, true);
    req.send(data);
  } catch(ex) {
    console.warn('POST failed: ' + ex.message);
  }
};

var serialize = function(obj) {
  var str = [];
  for(var p in obj)
    if (obj.hasOwnProperty(p)) {
      str.push(encodeURIComponent(p) + "=" + encodeURIComponent(obj[p]));
    }
  return str.join("&");
};

var translateConditionToFCTCode = function (condition) {
    var fctCode = 50;

    condition = condition.toLowerCase();
    var isHeavy = condition.indexOf('heavy') >= 0;
    var isLight = condition.indexOf('light') >= 0;
    condition = condition.replace('heavy ', '');
    condition = condition.replace('light ', '');

    switch(condition) {
        case 'clear':
            fctCode = 1;
            break;
        case 'partly cloudy':
            fctCode = 2;
            break;
        case 'mostly cloudy':
        case 'scattered clouds':
            fctCode = 3;
            break;
        case 'overcast':
            fctCode = 4;
            break;
        case 'mist':
        case 'fog':
        case 'fog patches':
        case 'smoke':
        case 'haze':
        case 'freezing fog':
        case 'patches of fog':
        case 'shallow fog':
        case 'partial fog':
            fctCode = 6;
            break;
        case 'drizzle':
        case 'rain mist':
            fctCode = 10;
            break;
        case 'rain':
        case 'rain showers':
        case 'unknown precipitation':
            fctCode = 13;
            break;
        case 'thunderstorm':
        case 'thunderstorms and rain':
        case 'thunderstorms and snow':
        case 'thunderstorms and ice pellets':
        case 'thunderstorms with hail':
        case 'thunderstorms with small hail':
        case 'squalls':
        case 'funnel cloud':
            fctCode = 15;
            break;
        case 'ice crystals':
        case 'ice pellets':
            fctCode = 18;
            break;
        case 'snow':
        case 'snow grains':
        case 'low drifting snow':
        case 'snow showers':
        case 'snow blowing snow mist':
            fctCode = 21;
            break;
        case 'hail':
        case 'hail showers':
        case 'small hail':
            fctCode = 23;
            break;
        case 'volcanic ash':
        case 'widespread dust':
        case 'sand':
        case 'spray':
        case 'dust whirls':
        case 'sandstorm':
        case 'low drifting widespread dust':
        case 'low drifting sand':
        case 'blowing widespread dust':
        case 'blowing sand':
            fctCode = 30;
            break;
        case 'freezing drizzle':
        case 'freezing rain':
        case 'ice pellet showers':
        case 'small hail showers':
            fctCode = 31;
            break;
        case 'blowing snow':
            fctCode = 33;
            break;
        case 'unknown':
        default:
            fctCode = 50;
            break;
    }

    // Respect modifiers if they exist where possible
    if (fctCode === 13) {
        fctCode = isLight ? 10 : fctCode;
    }

    if (fctCode === 21) {
        fctCode = isHeavy ? 33 : fctCode;
    }

    return fctCode;
};
