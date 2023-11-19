import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpEventType, HttpRequest, HttpParams } from '@angular/common/http';
import { map, tap, last, catchError } from 'rxjs/operators';

import { FileMetadata, Pagination, FileUploadData} from './../classes/file-metadata';
import {BehaviorSubject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class FileService {
  fileUrl: string = "/api/files";
  public files: BehaviorSubject<FileMetadata[]> = new BehaviorSubject<FileMetadata[]>([]);

  public pagination: BehaviorSubject<Pagination> = new BehaviorSubject <Pagination>({
    ipp: 3,
    cp: 0,
    total: 20
  });
 

  constructor(private http: HttpClient) { }
  getFiles(): any{
    let ipp = 0;
    let cp = 0;
    this.pagination.subscribe((data)=>{
      ipp = data.ipp;
      cp = data.cp;
    });
    return this.http.get<{ files: FileMetadata[]}>(this.fileUrl +'?ipp='+ipp+'&cp='+cp);
  }
  upload(file: any,file_data : FileUploadData, dataHandler: (percentage: number, done: boolean, file_upload_id : number) => any) {
    var sendData = new FormData();
    sendData.append('file', file, file.name);
    
    const req = new HttpRequest(
      'POST',
      this.fileUrl,
      sendData,
      { reportProgress: true}
    );

    return this.http.request(req)
      .pipe(
        map(event => this.uploadStatus(event)),
        tap((status: { percentage: number, done: boolean}) => dataHandler(status.percentage, status.done, file_data.id)),
        last(),
        catchError(this.handleError)
      );
  }
  uploadStatus(event: any): { percentage: number, done: boolean } {
    if (event.body) {
    }

    let percentage = Math.round(100 * event.loaded / event.total);
    let done = HttpEventType.Response == event.type;

    return { percentage, done };
  }

  delete(file: FileMetadata): any {
    return this.http.delete(`${this.fileUrl}/${file._id}`)
      .pipe(
        tap(event => {
        }),
        catchError(this.handleError)
      );
  }

  private handleError(error: HttpErrorResponse) {
    if (error.error instanceof ErrorEvent) {
      // A client-side or network error occurred. Handle it accordingly.
      console.error('An error occurred:', error.error.message);
    } else {
      // The backend returned an unsuccessful response code.
      // The response body may contain clues as to what went wrong,
      console.error(
        `Backend returned code ${error.status}, ` +
        `body was: ${error.error}`);
    }

    // return an observable with a user-facing error message
    return  new String(
      'Something bad happened; please try again later.');


  };
}
