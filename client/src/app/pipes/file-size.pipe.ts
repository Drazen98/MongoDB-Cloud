import { Pipe, PipeTransform } from '@angular/core';

@Pipe({
  name: 'fileSize'
})
export class FileSizePipe implements PipeTransform {

  transform(bytes: number): string {
    var units = [
      'Bytes',
      'KB',
      'MB',
      'GB',
      'TB'
    ];
    var unit = 0;

    while (bytes >= 1024) {
      bytes /= 1024;
      unit++;
    }

    return +bytes.toFixed(2) + ' ' + units[unit];
  }

}
