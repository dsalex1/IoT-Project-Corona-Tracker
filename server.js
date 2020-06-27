var net = require('net');
const querystring = require('querystring');

const SERVER_PORT = 6000;
const SERVER_ADRESS ="0.0.0.0"

const NUM_HOSTS = 50

var receivedData = Array(NUM_HOSTS).fill("");
for (var i = SERVER_PORT; i < SERVER_PORT + NUM_HOSTS; i++)
    ((i)=>
    net.createServer((socket) => {
    socket.on('data', (data) => {
        receivedData[i] = receivedData[i] + data
        while (receivedData[i].indexOf("\n") >= 0) {
            if (receivedData[i].startsWith("POST /") || receivedData[i].startsWith("GET /")) {
                var req = receivedData[i].substring(0, receivedData[i].indexOf("\n"));
                socket.write(processRequest(req.split('\t\t')[0].split(" ")[0], req.split('\t\t')[0].split(" ")[1], req.split('\t\t').slice(1).join('\t\t')).slice(-124) + "\n")
            }
            receivedData[i] = receivedData[i].substring(receivedData[i].indexOf("\n") + 1, receivedData[i].length)
        }
    });
    }).listen(i, SERVER_ADRESS,() => {
        console.log("[SERVER] started on port " + i)
    }))(i)


var processRequest = function (method, path, body) {
    if (method=="POST")
        console.log(`[SERVER] received ${method} at ${path} with '${body}'`)

    if (method == "POST" && path.split("?")[0] == "/diagnosis-keys") return addDiagnosisKeys(body)
    if (method == "GET" && path.split("?")[0] == "/diagnosis-keys") return sendDiagnosisKeys(querystring.decode(path.split("?")[1]).ind) 
    return "";
}

var diagnosisKeys = []

var addDiagnosisKeys = function (keys) {
    //it is ok for the input to have trailing commas, remove them
    if (keys.endsWith(",")) keys = keys.slice(0, -1)
    diagnosisKeys = diagnosisKeys.concat(keys.split(","))
    return "SUCCESS";
}

var sendDiagnosisKeys = function (index) {
    return diagnosisKeys.slice(index).join("").slice(-124)
}

const readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
})

prompt = function(){
    readline.question(">", (input) => {
        if (input == "reset") {
            console.log("resetting stored diagnosis keys to [] ")
            diagnosisKeys = []
        }
        if (input == "clear") {
            console.log("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")
            console.log("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")
        }
        if (input == "keys") {
            console.log(diagnosisKeys)
        }
        if (input.startsWith("eval:")) {
            try {console.log(eval(input))
            }catch(e){console.log(e)}
        }
        prompt();
    })
}
prompt();