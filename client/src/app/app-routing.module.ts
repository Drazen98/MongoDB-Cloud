import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import {FilesComponent} from "./components/files/files.component";

const routes: Routes = [
  { path: "files", component: FilesComponent },
  { path: "**", component: FilesComponent }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
