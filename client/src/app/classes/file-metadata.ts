export class FileMetadata {
    constructor(
        public _id: string,
        public chunkSize: number,
        public contentType: string,
        public filename: string,
        public length: number,
        public md5: string,
        public uploadDate: string
    ) { }
}
export interface Pagination {
    ipp: number;
    cp: number;
    total: number;
};

export class FileUploadData{
    constructor(
    public filename : string,
    public id: number,
    public percentage : number
    )
    {}
}