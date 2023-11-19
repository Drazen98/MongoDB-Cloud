const router = require("express").Router();
const FileController = require("./../db").FileController;


function filesRouter(dbConnection){
    router.route('/api/files')
        .get(FileController.getFiles)
        .post(FileController.upload);
    router.route('/api/files/:id')
        .get(FileController.download)
        .delete(FileController.delete);
    return router;
}

module.exports = {filesRouter};