const  config = require("./config");
const express = require("express");
const bodyParser = require("body-parser");
const morgan = require('morgan');
const busboyBodyParser = require("busboy-body-parser");

let app = express();
app.use(morgan('tiny'));

app.use(bodyParser.urlencoded({
    extended: false
}));
app.use(bodyParser.json());
app.use(busboyBodyParser());

async function createServer(){
    await app.listen(config.port, () => {
        console.log("I'm listening at " + config.port);
    });
    return app;
}

module.exports = {createServer};