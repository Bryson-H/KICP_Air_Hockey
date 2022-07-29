// JJROBTOS AHR AIR HOCKEY ROBOT EVO PROJECT

// Each time a new data packet from camera is reveived this function is called
void newDataStrategy()
{
  // predict_status == 0 => No risk
  // predict_status == 1 => Puck is moving to our field directly
  // predict_status == 2 => Puck is moving to our field with a bounce
  // predict_status == 3 => ?

  // Default
  robot_status = 0;   // Going to initial position (defense)

  if (predict_status == 1) // Puck comming?
  {
    if (predict_bounce == 0)  // Direct impact?
    {
      if ((predict_x > (ROBOT_MIN_X + 50)) && (predict_x < (ROBOT_MAX_X - 50)))
      {
        if (puckSpeedYAverage > -250)
          robot_status = 2;  // defense+attack
        else
          robot_status = 1;  // Puck too fast => only defense
      }
      else
      {
        if (predict_time < 400)
          robot_status = 1; //1  // Defense
        else
          robot_status = 0;
      }
    }
    else // Puck come from a bounce?
    {
      if (puckSpeedYAverage > -160) // Puck is moving fast?
        robot_status = 2;  // Defense+Attack
      else
        robot_status = 1;  // Defense (too fast...)
    }

  }

  // Prediction with side bound
  if (predict_status == 2)
  {
    // Limit movement
    predict_x = constrain(predict_x, ROBOT_CENTER_X - (PUCK_SIZE * 4), ROBOT_CENTER_X + (PUCK_SIZE * 4));
    robot_status = 1;   // only defense mode
  }

  // If the puck is moving slowly in the robot field we could start an attack
  if ((predict_status == 0) && (puckCoordY < (ROBOT_CENTER_Y - 20)) && (myAbs(puckSpeedY) < 45))
  {
    robot_status = 3;
  }

}

// Robot Moves depends directly on robot status
// robot status:
//   0: Go to defense position
//   1: Defense mode (only move on X axis on the defense line)
//   2: Defense + attach mode
//   3: Attack mode
//   4: ?? REMOVE ??
//   5: Manual mode => User send direct commands to robot
void robotStrategy()
{
  max_speed = user_max_speed;  // default to max robot speed and accel  
  max_acceleration = user_max_accel;  
  switch (robot_status) {
    case 0:
      // Go to defense position
      com_pos_y = defense_position;
      com_pos_x = ROBOT_CENTER_X;  //center X axis
      max_speed = (user_max_speed / 3) * 2; // Return a bit more slowly...      
      setPosition_straight(com_pos_x, com_pos_y);
      attack_time = 0;
      break;
    case 1:
      // Defense mode (only move on X axis on the defense line)
      predict_x = constrain(predict_x, (PUCK_SIZE * 3), TABLE_WIDTH - (PUCK_SIZE * 3));  // we leave some space near the borders...
      com_pos_y = defense_position;
      com_pos_x = predict_x;
      setPosition_straight(com_pos_x, com_pos_y);
      attack_time = 0;
      break;
    case 2:
      // Defense+attack
      if (predict_time_attack < 150) // If time is less than 150ms we start the attack HACELO DEPENDIENTE DE LA VELOCIDAD?? NO, solo depende de cuanto tardemos desde defensa a ataque...
      {
        com_pos_y = attack_position + PUCK_SIZE * 4; // we need some override
        com_pos_x = predict_x_attack;
        setPosition_straight(com_pos_x, com_pos_y);
      }
      else      // Defense position
      {
        com_pos_y = predict_y;
        com_pos_x = predict_x;  // predict_x_attack;
        setPosition_straight(com_pos_x, com_pos_y);
        attack_time = 0;
      }

      break;
    case 3:
      // ATTACK MODE
      if (attack_time == 0)
      {
        attack_predict_x = predictPuckXPosition(500);
        attack_predict_y = predictPuckYPosition(500);
        if ((attack_predict_x > (PUCK_SIZE * 3)) && (attack_predict_x < (TABLE_WIDTH - (PUCK_SIZE * 3))) && (attack_predict_y > (PUCK_SIZE * 4)) && (attack_predict_y < (ROBOT_CENTER_Y - (PUCK_SIZE * 4))))
        {
          attack_time = millis() + 500;  // Prepare an attack in 500ms
          attack_pos_x = attack_predict_x;  // predict_x
          attack_pos_y = attack_predict_y;  // predict_y
          //Serial.print("AM:");
          //Serial.print(attack_time);
          //Serial.print(",");
          //Serial.print(attack_pos_x);
          //Serial.print(",");
          //Serial.println(attack_pos_y);
          //Serial.print(" ");
          // Go to pre-attack position
          com_pos_x = attack_pos_x;
          com_pos_y = attack_pos_y - PUCK_SIZE * 3;
          max_speed = user_max_speed / 2;          
          setPosition_straight(com_pos_x, com_pos_y);
          attack_status = 1;
        }
        else
        {
          attack_time = 0;  // Continue waiting for the right attack moment...
          attack_status = 0;
          // And go to defense position
          com_pos_y = defense_position;
          com_pos_x = ROBOT_CENTER_X;  //center X axis
          max_speed = (user_max_speed / 3) * 2;          
          setPosition_straight(com_pos_x, com_pos_y);
        }
      }
      else
      {
        if (attack_status == 1)
        {
          if ((attack_time - millis()) < 200)  // less than 200ms to start the attack
          {
            // Attack movement
            com_pos_x = predictPuckXPosition(200);
            com_pos_y = predictPuckYPosition(200);
            setPosition_straight(com_pos_x, (com_pos_y + PUCK_SIZE * 2));

            //Serial.print("ATTACK:");
            //Serial.print(com_pos_x);
            //Serial.print(",");
            //Serial.println(com_pos_y);

            attack_status = 2; // Attacking
          }
          else  // attack_status=1 but itÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â´s no time to attack yet
          {
            // Go to pre-attack position
            com_pos_x = attack_pos_x;
            com_pos_y = attack_pos_y - PUCK_SIZE * 3;
            max_speed = user_max_speed / 2;            
            setPosition_straight(com_pos_x, com_pos_y);
          }
        }
        if (attack_status == 2)
        {
          if (millis() > (attack_time + 80)) // Attack move is done? => Reset to defense position
          {
            //Serial.print("RESET");
            attack_time = 0;
            robot_status = 0;
            attack_status = 0;
          }
        }
      }
      break;
    case 4:
      // The puck came from a bounce
      // Only defense now (we could improve this in future)
      // Defense mode (only move on X axis on the defense line)
      predict_x = constrain(predict_x, (PUCK_SIZE * 3), TABLE_WIDTH - (PUCK_SIZE * 3));
      com_pos_y = defense_position;
      com_pos_x = predict_x;
      setPosition_straight(com_pos_x, com_pos_y);
      attack_time = 0;
      break;

    case 5:
      // User manual control
      max_speed = user_target_speed;      
      // Control acceleration
      max_acceleration = user_target_accel;      
      setPosition_straight(user_target_x, user_target_y);
      //Serial.println(max_acceleration);
      break;

    default:
      // Default : go to defense position
      com_pos_y = defense_position;
      com_pos_x = ROBOT_CENTER_X; // center
      setPosition_straight(com_pos_x, com_pos_y);
      attack_time = 0;
  }
}

void testMovements()
{
  if (loop_counter >= 32000){
    //Serial.println("Test pattern...");
    testmode=false;
    return;
  }
  max_speed = user_max_speed;
  if (loop_counter > 12000) 
    setPosition_straight(ROBOT_INITIAL_POSITION_X, ROBOT_INITIAL_POSITION_Y);
  else if (loop_counter > 10500) 
    setPosition_straight(ROBOT_MAX_X, ROBOT_MIN_Y);
  else if (loop_counter > 9000)        
    setPosition_straight(ROBOT_MIN_X, ROBOT_MAX_Y);
  else if (loop_counter > 7500) 
    setPosition_straight(TABLE_WIDTH/2, ROBOT_MAX_Y);
  else if (loop_counter > 6000) 
    setPosition_straight(ROBOT_INITIAL_POSITION_X, ROBOT_INITIAL_POSITION_Y);
  else if (loop_counter > 4500) 
    setPosition_straight(ROBOT_MIN_X, ROBOT_MIN_Y);
  else if (loop_counter > 3000)        
    setPosition_straight(ROBOT_MAX_X, ROBOT_MAX_Y);
  else if (loop_counter > 1500) 
    setPosition_straight(TABLE_WIDTH/2, ROBOT_MAX_Y);
  else
    setPosition_straight(ROBOT_INITIAL_POSITION_X, ROBOT_INITIAL_POSITION_Y);
}
