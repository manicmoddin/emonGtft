void setupSolarMonitor() {
  drawCrossHair();
  drawButtons(3);

  //draw the main Box titles
  /* Consuming */
  tft.setCursor(5,5);
  tft.setTextColor(0xffff);
  tft.setTextSize(3);
  tft.print("Using Now");

  /* Generating */
  tft.setCursor(190, 5);
  tft.print("Generating");

  /* Inport / Eport */
  tft.setCursor(15, 170);
  tft.print("In / Out");

  /* Temp */
  tft.setCursor(195, 170);
  tft.print("Room Temp");

}

void drawSolarMonitor(double gen, double oldGen, double use, double oldUse, double grid, double oldGrid, double tempLocal, double oldTempLocal, int tempMin, int tempMax) {

  //first check to see if the page has changed
  if(oldPage != page) {
    stopInterrupt();
    clearScreen();
    setupSolarMonitor();
    writeTime();
    startInterrupt();
    oldPage = page;
    oldGen = 1;
    oldUse = 1;
    oldGrid = 1;
    oldTemp = 1;
  }

  //only paint the new values, if the values are different.
  tft.setTextSize(5);
  char str[50];
  /* Using */
  if(use != oldUse) {
    stopInterrupt();
    //black out the old value
    tft.setCursor(25,tft.height() /4);
    tft.setTextColor(0x0000);
    //    tft.print(oldUse);
    if(oldUse >= 1000) {
      dtostrf(oldUse/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)oldUse,str,10);
      strcat(str,"w");  
    }
    tft.print(str);

    //now print the new value
    if(use <300 ) {
      tft.setTextColor(GREEN);
    }

    if (use >=300 & use <=700) {
      tft.setTextColor(YELLOW);
    }

    if(use >700) {
      tft.setTextColor(RED);
    }
    tft.setCursor(25,tft.height() /4);
    if(use >= 1000) {
      dtostrf(use/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)use,str,10);
      strcat(str,"w");  
    }
    tft.print(str);
    startInterrupt();
  }

  /* Generating */
  if( gen != oldGen ) {
    stopInterrupt();
    //blackout the old reading
    tft.setCursor(215, tft.height() /4);
    tft.setTextColor(BLACK);
    if(oldGen >= 1000) {
      dtostrf(oldGen/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)oldGen,str,10);
      strcat(str,"w");  
    }
    tft.print(str);

    //now print new value
    if(gen <150) {
      tft.setTextColor(RED);
    }
    else if(gen >= 150 & gen <1000) {
      tft.setTextColor(YELLOW);
    }
    else {
      tft.setTextColor(GREEN);
    }
    tft.setCursor(215, tft.height() /4);
    if(gen >= 1000) {
      dtostrf(gen/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)gen,str,10);
      strcat(str,"w");  
    }
    tft.print(str);

    startInterrupt();
  }

  /* Import / export */
  if( grid != oldGrid ) {
    stopInterrupt();

    //blackout the old reading
    tft.setCursor(25, (tft.height() / 4) * 3);
    tft.setTextColor(BLACK);
    //tft.print(oldGrid);
    if(oldGrid<-1000 || grid>1000)
    {
      dtostrf(oldGrid/1000,2,1,str);
      strcat(str,"kw");   
    }
    else
    {
      itoa((int)oldGrid,str,10);
      strcat(str,"w");   
    }
    tft.print(str);

    //write the new
    tft.setCursor(25, (tft.height() / 4) * 3);
    tft.setTextColor(WHITE);
    if(grid<-1000 || grid>1000)
    {
      dtostrf(grid/1000,2,1,str);
      strcat(str,"kw");   
    }
    else
    {
      itoa((int)grid,str,10);
      strcat(str,"w");   
    }
    tft.print(str);
    startInterrupt();
  }

  /* Temp */
  if (tempLocal != oldTempLocal) {
    stopInterrupt();
    //blackout the old
    tft.setCursor(215, (tft.height() / 4) * 3);
    tft.setTextColor(BLACK);
    //tft.print(oldTemp);
    //tft.print(char(247));
    //tft.print("C");
    dtostrf(oldTemp,0,1,str); 
    strcat(str,"c");
    tft.print(str);

    //write the new
    tft.setCursor(215, (tft.height() / 4) * 3);
    tft.setTextColor(WHITE);
    dtostrf(temp,0,1,str); 
    strcat(str,"c");
    tft.print(str);
    startInterrupt();
    oldTemp = temp;        //addded so that the temp stops strobing
    Serial.print(temp); Serial.print(" "); Serial.println(oldTemp);
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

void setupWaterPage() {
  drawButtons(3);
  int width = tft.width() - buttonSpaceH;
  int tankRadius = 75;
  //tft.setCursor(width - tankRadius);
  tft.drawCircle((width - tankRadius) -15, tankRadius + 15, tankRadius, 0xffff);
  tft.drawCircle((width - tankRadius) -15, (tft.height() - tankRadius) -15, tankRadius, 0xffff);
  tft.fillRect((width - (tankRadius)*2) -15, tankRadius +15, tankRadius*2 +1,tankRadius*2, BLACK);
  tft.drawFastVLine((width - (tankRadius*2)) -15, tankRadius +15, tankRadius * 2, WHITE);
  tft.drawFastVLine(width - 15, tankRadius +15, tankRadius*2, WHITE); 
}

void drawWaterPage(int top, int bottom) {

  //first check to see if the page has changed
  if(oldPage != page) {
    stopInterrupt();
    clearScreen();
    writeTime();
    setupWaterPage();
    startInterrupt();
    oldPage = page;
    oldTankTop = 1;
    oldTankBottom = 1;
  }

  //this will always be a slow update, except of first boot.

  int topPositionX = tft.width() - buttonSpaceH - 115;
  int topPositionY = 45;

  if(top != oldTankTop) {
    //update the screen as the temperature has changed.

    stopInterrupt();
    tft.setTextSize(4);
    tft.setCursor(topPositionX, topPositionY);
    tft.setTextColor(BLACK);
    tft.print(oldTankTop);
    oldTankTop = top;
    tft.setCursor(topPositionX, topPositionY);
    if(top > 35) {
      tft.setTextColor(RED);
    }
    else {
      tft.setTextColor(CYAN);
    }
    tft.print(top);
    startInterrupt();

  }

  //update the bottom Temp if it has changed
  if(bottom != oldTankBottom) {
    stopInterrupt();
    tft.setTextSize(4);
    tft.setCursor(topPositionX, tft.height() - 75);
    tft.setTextColor(BLACK);
    tft.print(oldTankBottom);
    oldTankBottom = bottom;
    tft.setCursor(topPositionX, tft.height() - 75);
    if(bottom > 35) {
      tft.setTextColor(RED);
    }
    else {
      tft.setTextColor(CYAN);
    }
    tft.print(bottom);
    startInterrupt();
  }



}


////////////////////////////////////////////////////////////////////////////////////

//Consumption Only

void setupGridMonitor() {
  //drawCrossHair();
  drawButtons(3);

  //draw the main Box titles
  /* Consuming */
  //  tft.setCursor(5,5);
  //  tft.setTextColor(0xffff);
  //  tft.setTextSize(3);
  //  tft.print("Using Now");
  //  
  //  /* Generating */
  //  tft.setCursor(190, 5);
  //  tft.print("Generating");
  //  
  //  /* Inport / Eport */
  //  tft.setCursor(15, 170);
  //  tft.print("In / Out");
  //  
  //  /* Temp */
  //  tft.setCursor(195, 170);
  //  tft.print("Room Temp");

}

void drawGridMonitor(int fUse, int fOldUse, double fTemp, double fOldTemp) {

  //first check to see if the page has changed
  if(oldPage != page) {
    stopInterrupt();
    clearScreen();
    writeTime();
    setupGridMonitor();
    startInterrupt();
    oldPage = page;
    fOldUse = 1;
    oldTemp = 1;
  }
  
  char str[10];
  //has the usage changed?
  if(fUse != fOldUse) {
    tft.setTextSize(10);
    stopInterrupt();
    //black out the old value
    tft.setCursor(50,tft.height() /4);
    tft.setTextColor(0x0000);
    if(fOldUse >= 1000) {
      dtostrf(fOldUse/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)fOldUse,str,10);
      strcat(str,"w");  
    }
    tft.print(str);

    //now print the new value
    if(fUse <300 ) {
      tft.setTextColor(GREEN);
    }

    if (fUse >=300 & fUse <=700) {
      tft.setTextColor(YELLOW);
    }

    if(fUse >700) {
      tft.setTextColor(RED);
    }
    tft.setCursor(50,tft.height() /4);
    if(fUse >= 1000) {
      dtostrf(fUse/1000,2,1,str);
      strcat(str,"kw");
      //use = use / 1000;

      //tft.print("kW");
    }
    else{
      itoa((int)fUse,str,10);
      strcat(str,"w");  
    }
    tft.print(str);
    old_use = fUse;
    startInterrupt();
  }  

  /* Temp */
  if (temp != fOldTemp) {
    stopInterrupt();
    tft.setTextSize(5);
    //blackout the old
    tft.setCursor(225, (tft.height() / 4) * 3);
    tft.setTextColor(BLACK);
    tft.print(fOldTemp);
    tft.print(char(247));
    tft.print("C");

    //write the new
    tft.setCursor(225, (tft.height() / 4) * 3);
    tft.setTextColor(WHITE);
    tft.print(temp);
    tft.print(char(247));
    tft.print("C"); 
    //attachInterrupt(0, rf12_interrupt, LOW);
    //rf12_initialize(MYNODE, freq,group);
    //Serial.println(oldTemp);
    //Serial.println(temp);
    oldTemp = temp;
    startInterrupt();
  }
}


void clearScreen() {
  tft.fillScreen(0x0000);
}



