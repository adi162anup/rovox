const express = require('express');
const alexa = require('alexa-app');
const dotenv = require('dotenv');

dotenv.config();

const port = process.env.PORT;
const app = express();

// end point defined to handle requests coming from alexa. "move" is the name of the endpoint

const alexaApp = new alexa.app("move")

// endpoint is linked to express router

alexaApp.express({
    expressApp: app,

    checkCert: false,

    debug: true
})

alexaApp.launch((req,res) => {
    res.say("App is launched!");
})

// intent as well as utterances defined

alexaApp.intent('Start', {
    "utterances": ["Start the rover"]
},(req,res) => {
    res.say("Rover has started")
})

app.listen(port, () => {
    console.log(`Server listening on port ${port}`);
})