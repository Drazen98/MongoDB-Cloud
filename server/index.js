const createServer  = require("./express").createServer;
const connectToMongo = require("./db").connectToMongo;
const filesRouter = require("./routes/files.router").filesRouter;

(async () => {
    const db = await connectToMongo();
    const app = await createServer();
    
    app.use(filesRouter(db));
}
)();






