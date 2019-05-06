void printDirectory() {
  root = SD.open("/");
  byte i = 0;
  byte j = 0;
  while (i < 4) {
    dataFile =  root.openNextFile();
    j++;
    if (!dataFile) {
      while (i < 4){
        fileIndex[i]=0;
        i++;
      }
      break;
    }
    if (!dataFile.isDirectory()) {
      fileIndex[i] = {j};
      lcd.setCursor(1, i);
      lcd.print(dataFile.name());
      i++;
    }
    dataFile.close();
  }
  root.close();
}


void openFileIndex(byte _index){
  root = SD.open("/");
  byte i = 0;
  byte j = 0;
  while (j < _index) {
    dataFile =  root.openNextFile();
    j++;
    if (!dataFile) {
      break;
    }
    if (j < _index) dataFile.close();
  }
  fileSize = dataFile.size();
  fileProg = 0;
  root.close();
  menuPosition = 2;
  readFromSD = 1;
}

void endFileJob(){
  readFromSD = 0;
  dataFile.close();
  menuPosition = 2;
  setPause(0);
}
