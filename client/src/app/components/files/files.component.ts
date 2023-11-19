import { Component, OnInit, TemplateRef } from '@angular/core';
import { faFileUpload, faFileDownload, faTrash, faSearch } from '@fortawesome/free-solid-svg-icons';
import { FormControl, FormGroup, Validators } from '@angular/forms';
import { BsModalService, BsModalRef } from 'ngx-bootstrap/modal';
import { FileService } from './../../services/file.service';
import { FileMetadata, Pagination, FileUploadData } from './../../classes/file-metadata';
import { BooleanInput } from 'ngx-bootstrap/focus-trap/boolean-property';



@Component({
  selector: 'app-files',
  templateUrl: './files.component.html',
  styleUrls: ['./files.component.css']
})
export class FilesComponent implements OnInit {
  faFileUpload = faFileUpload;
  faFileDownload = faFileDownload;
  faFileSearch =faSearch;
  faTrash = faTrash;
  modalRef?: BsModalRef;
  
  fileToDelete : FileMetadata =new FileMetadata("",0,"","",0,",","");
  files : FileMetadata[] = [];
  list_uploading: FileUploadData[] = [];
  upload_id : number = 0;
  pagination: Pagination = {
    ipp: 3,
    cp: 0,
    total: 0
  };
  fileForm = new FormGroup({
    file: new FormControl(null, [Validators.required])
  });
 
 
  constructor(private modalService: BsModalService, public fileService: FileService) { }
  parseString(title: string){
    return (title.slice(0,21)+"...");
  }
  dataHandler(percentage: number, done: BooleanInput, file_id : number): void {
    if (!isNaN(percentage)) {
      for(var i=0;i<this.list_uploading.length; i++){
        if (this.list_uploading[i].id==file_id ){
          this.list_uploading[i].percentage = percentage;
          break;
        } 
      }
    }

    if (done) {
      for (var i = 0; i < this.list_uploading.length; i++) {
        if (this.list_uploading[i].id == file_id) {
          this.list_uploading.splice(i,1);
          break;
        }
      }
    }
 
  }
  upload() {
    let new_file_name = this.parseString(this.fileForm.controls["file"].value.name);
    let file_data: FileUploadData = new FileUploadData(new_file_name, this.upload_id, 0);
    this.upload_id +=1;
    if(this.upload_id==100){
      this.upload_id = 0;
    }
    
    this.list_uploading.push(file_data);
    this.closeModal();
    this.fileService.upload(
      this.fileForm.controls["file"].value,
      file_data,
      this.dataHandler.bind(this)
    ).subscribe({
      error: (e) => { console.log(e); this.closeModal(); },
      complete: () => {
        this.ngOnInit();
      }
    });
  }

  openDeleteWarning(file: FileMetadata, template: TemplateRef<any>) {
    this.fileToDelete = file;
    this.openModal(template);

  }
  changeipp(event: any){

    let tmpcp = this.pagination.cp;
    if (Math.ceil(this.pagination.total / event.target.value)< tmpcp)
      tmpcp = Math.ceil(this.pagination.total / event.target.value);
   
    let pagination = {
      ipp: event.target.value,
      cp: tmpcp,
      total: this.pagination.total
    };
    this.fileService.pagination.next(pagination);
    this.pagination = pagination;
    this.getFiles();
  }
  changePage(event : any){
    let pagination= {
      ipp: this.pagination.ipp,
      cp: event.page-1,
      total: this.pagination.total
    };
    this.fileService.pagination.next(pagination);
    this.pagination = pagination;
    this.getFiles();
  }
  deleteFile(){
    this.fileService.delete(this.fileToDelete).subscribe({
      error: (e:any) => { console.log(e);},
      complete: () => {
        this.closeModal();
        let pagination = {
          ipp: this.pagination.ipp,
          cp: this.pagination.cp,
          total: this.pagination.total - 1
        };
        this.fileService.pagination.next(pagination);
        this.pagination = pagination;
        this.getFiles();}
    });
  }
  
  onFileChange(event : any): any {
    if (event.target.files && event.target.files.length){
      this.fileForm.patchValue({
        file: event.target.files.item(0)
      });}
    }
  
  
  openModal(template: TemplateRef<any>) {
    this.modalRef = this.modalService.show(template);
  }

  closeModal(){
    this.modalRef?.hide();
  }
  getFiles(){
    this.fileService.getFiles().subscribe((data: any) => {
      this.fileService.files.next(data.files);
      this.files = data.files;
    });
  }
  ngOnInit(): void {
    this.fileService.getFiles().subscribe((data : any) => {
      let pagination = this.pagination;
      this.fileService.pagination.subscribe((data)=>{
        pagination.ipp = data.ipp;
        pagination.cp = data.cp;
      });
      pagination.total = data.filen;
      this.fileService.pagination.next(pagination);
      this.pagination = pagination;
      
      this.fileService.files.next(data.files);
      this.files = data.files;
  });
  }
}
