const mongoose = require("mongoose");
const Readable = require("stream").Readable;
const config = require("./config");

const contentDisposition = require('content-disposition');


const objectId = mongoose.Schema.Types.ObjectId;

const fileSchema = new mongoose.Schema({
    length: { type: Number, required: true },
    chunkSize: { type: Number, required: true },
    uploadDate: { type: Date, required: true },
    filename: { type: String, required: true },
    md5: { type: String, required: true },
    contentType: { type: String, required: true }
}, { strict: false });


const chunkSchema = new mongoose.Schema({
    files_id: { type: objectId, required: true },
    n: { type: Number, required: true },
    data: { type: Buffer }
}, { strict: false });



var File;
var Chunk;
var bucket;

class FileController {
    static async upload(req, res) {
       
        let file = req.files.file;
        let contentType = file.mimetype;
        let filename = file.name;
     
        
        const readableStream = new Readable();
        readableStream.push(file.data);
        readableStream.push(null);

        let uploadStream = bucket.openUploadStream(filename, { contentType });
        readableStream.pipe(uploadStream);

        let message;
        uploadStream.on("error", () => {
            message = "Error when uploading";
            return res.status(500).json({ message });
        });

        uploadStream.on("finish", async () => {
            try {
                message = "File uploaded";
                let fileInfo = await FileService.findFileById(uploadStream.id);
                return res.status(201).json({ message, fileInfo });
            } catch (err) {
                return res.status(500).json(err);
            }
        });
    }
    static async getFiles(req, res) {
        let files;
        let filen;
        try {
            files = await FileService.findFiles(Number(req.query.ipp), Number(req.query.cp));
        } catch (err) {
            console.log("err");
            return res.status(500).json(err);
        }
        
        try {
            filen = await FileService.numberOfFiles();
        } catch (err) {
            return res.status(500).json(err);
        }
        let message = "Sending files";
        return res.status(200).send({ files, filen, message });

    }
    static async download(req, res) {
        let id = req.params.id;
        let file, message;

        try {
            file = await FileService.findFileById(id);
        } catch (err) {
            return res.status(404).json({ message });
        }
        if (!file) {
            message = "Not found";
            return res.status(404).json({ message });
        }
        let downloadStream = bucket.openDownloadStream(mongoose.Types.ObjectId(id));
       
    
        res.writeHead(200, { 
            "Content-Disposition": contentDisposition(file.filename),
            "Content-Type": file.contentType });
        delete res.headers;
        downloadStream.pipe(res);

    }
    static async delete(req, res) {
        let id = req.params.id;
        let message, file;
        try {
            file = await FileService.deleteFilesById(id);
        } catch (err) {
            return res.status(404).json({ message });
        }
        if (!file) {
            message = "File not found";
            return res.status(200).json({ message });
        }
        message = "Successfully removed";
        return res.status(200).json({ message });
    }

}

class FileService {
    static async findFileById(_id) {
        return await File.findOne({_id}).exec();
    }
    static async findFiles(ipp, cp){
        return await File.find().skip(cp*ipp).limit(ipp).exec();
    }
    static async deleteFilesById(id) {
        await Chunk.deleteMany({ files_id: id }).exec();
        return await File.deleteOne({ _id: id }).exec();
    }
    static async numberOfFiles() {
        return await File.find().countDocuments().exec();
    }
}
async function connectToMongo() {
    const conn = await mongoose.createConnection(config.dbURI, { useNewUrlParser: true, useUnifiedTopology: true }).
        asPromise();
    conn.set("useCreateIndex", true);
    if(conn.readyState){
        conn.model("File", fileSchema, "fs.files");
        conn.model("Chunk", chunkSchema, "fs.chunks");

        File = conn.model("File");
        Chunk = conn.model("Chunk");
        bucket = new mongoose.mongo.GridFSBucket(conn.db);
    }
    return conn;
}

module.exports = {connectToMongo,FileController};