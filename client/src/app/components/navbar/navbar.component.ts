import { Component, OnInit } from '@angular/core';
import { Router } from '@angular/router';

@Component({
  selector: 'app-navbar',
  templateUrl: './navbar.component.html',
  styleUrls: ['./navbar.component.css']
})
export class NavbarComponent implements OnInit {
  public url1: string = "";
  constructor(private router: Router) {
    router.events.subscribe((val) => { this.url1 = this.router.url; });
   }

  ngOnInit(): void {
    this.url1 = this.router.url;
  }

}
