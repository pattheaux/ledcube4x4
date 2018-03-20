
int checkButton(struct btn *b) {
  int nstate = !digitalRead(b->pin);
  if((nstate != b->state) && (b->stime == 0)) {
    b->stime = millis();
  } else if(nstate != b->state) {
    if(millis() - b->stime > 50) {
      //digitalWrite(LED_BUILTIN, nstate?HIGH:LOW);
      b->state = nstate;
      b->stime = 0;
      
      if(b->state) b->_pstart = millis();
      else b->ptime = millis() - b->_pstart;  // Total press time
      
      b->changed = true;
      
      //if(!state) Serial.println("*");
      //else Serial.print("V");
    }
  } else {
    b->stime = 0;
  }

  if(b->state) {
    b->ptime = millis() - b->_pstart;  // Press time so far
  }

  return b->state;
}

