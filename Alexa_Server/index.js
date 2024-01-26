const express = require('express');
const alexa = require('alexa-app');
const dotenv = require('dotenv');

dotenv.config();

const port = process.env.PORT;
const app = express();

app.listen(port, () => {
    console.log(`Server listening on port ${port}`);
})