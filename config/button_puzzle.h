#pragma once


/*
The module is armed when the light is turned on.
The module is disarmed when the light is turned off.
When the light is turned off, arm the module by pressing the button.
When the light is blue and the last digit of the timer is 7, press and release immediately.
When the light is red, press and hold.
When the light is green, press and hold.
When the light is purple, press and hold.
When the light is blinking blue, release when the last digit of the timer is 5.
When the light is blinking red, release when the last digit of the timer is 3.
When the light is blinking green, release when the last digit of the timer is 6.
When the light is blinking purple, release when the last digit of the timer is 2.
When the light is blue and blinking fast, press and release immediately when the last digit of the timer is 8.
When the light is red and blinking fast, press and release immediately when the last digit of the timer is 1.
When the light is green and blinking fast, press and release immediately when the last digit of the timer is 9.
When the light is purple and blinking fast, press and release immediately when the last digit of the timer is 7.

The module is armed when the light is turned on.
The module is in a disarmed state when the light is not lit.
To arm the module when disarmed, simply press the button.
Press the button and release it instantly if the light is blue and the timer’s last digit happens to be 7.
A red light indicates that the button must be pressed and hold.
When the light is solid green, press the button and keep it pressed.
When the light is solid purple, press the button and keep it pressed.
Release the button when the final digit of the timer shows 5 and the light is blinking blue.
With a blinking red light, let go of the button once the last digit becomes 3.
A flashing green light means you should release the button when the timer ends in 6.
A flashing purple light means you should release the button when the timer ends in 2.
When the light is flashing rapidly in blue, wait until the timer’s last digit is 8 and then perform a quick press-and-release.
If the light blinks rapidly in green, press and release immediately when the final digit shows 9.
For a fast-blinking red light, do a quick tap-and-release at the moment the timer ends in 1.
If the light blinks rapidly in purple, press and release immediately when the final digit shows 7.

De module is geactiveerd wanneer het lampje is ingeschakeld.
De module bevindt zich in een gedeactiveerde toestand wanneer het lampje niet brandt.
Om de module te activeren wanneer deze gedeactiveerd is, druk je eenvoudig op de knop.
Druk op de knop en laat deze onmiddellijk los als het lampje blauw is en het laatste cijfer van de timer een 7 is.
Een rood lampje geeft aan dat de knop ingedrukt gehouden moet worden.
Wanneer het lampje egaal groen brandt: druk op de knop en houd deze ingedrukt.
Wanneer het lampje egaal paars brandt: druk op de knop en houd deze ingedrukt.
Laat de knop los wanneer het laatste cijfer van de timer een 5 toont en het lampje blauw knippert.
Bij een knipperend rood lampje laat je de knop los zodra het laatste cijfer een 3 wordt.
Een knipperend groen lampje betekent dat je de knop moet loslaten wanneer de timer eindigt op 6.
Een knipperend paars lampje betekent dat je de knop moet loslaten wanneer de timer eindigt op 2.
Wanneer het lampje snel blauw knippert, wacht je tot het laatste cijfer van de timer een 8 is en voer je vervolgens een snelle druk-en-los actie uit.
Als het lampje snel groen knippert, druk en laat dan onmiddellijk los wanneer het laatste cijfer een 9 aangeeft.
Voor een snel knipperend rood lampje doe je een korte tik-en-los op het moment dat de timer eindigt op 1.
Als het lampje snel paars knippert, druk en laat dan onmiddellijk los wanneer het laatste cijfer een 7 aangeeft.
*/

namespace ButtonPuzzle {

// Initialize puzzle state (stage 1 with random color: 0=blue, 1=green, 2=red, 3=purple)
void initialize(int& button_stage, int& led_color, esphome::light::LightCall& call) {
    button_stage = 1;
    led_color = esp_random() % 4;
    call.set_state(true);
    call.set_effect("None");
    if (led_color == 0) {
        call.set_rgb(0, 0, 1);  // Blue
    } else if (led_color == 1) {
        call.set_rgb(0, 1, 0);  // Green
    } else if (led_color == 2) {
        call.set_rgb(1, 0, 0);  // Red
    } else if (led_color == 3) {
        call.set_rgb(1, 0, 1);  // Purple
    }
    call.perform();
}

// Reset puzzle to stage 1
void reset(int& button_stage, int& led_color, esphome::light::LightCall& call) {
    ESP_LOGW("ButtonPuzzle", "Reset called - BOMB EXPLODED!");
    // Trigger global explode handling script with reason
    id(explode).execute("Verkeerde knop");
    // Turn off LED locally
    call.set_state(false);
    call.perform();
}

// Handle short press
void handle_short_press(int& button_stage, int& led_color, int game_timer, esphome::light::LightCall& call, bool& puzzle_solved) {
    // Stage 1: Non-blinking
    if (button_stage == 1) {
        // Blue + short press at digit 7 = stage 3 (fast blinking)
        if (led_color == 0 && game_timer % 10 == 7) {
            ESP_LOGW("ButtonPuzzle", "Blue + short press at 7 = stage 3 (fast blinking)");
            button_stage = 3;
            led_color = esp_random() % 4;
            if (led_color == 0) {
                call.set_effect("blink_blue_fast");
            } else if (led_color == 1) {
                call.set_effect("blink_green_fast");
            } else if (led_color == 2) {
                call.set_effect("blink_red_fast");
            } else if (led_color == 3) {
                call.set_effect("blink_purple_fast");
            }
            call.perform();
            return;
        }

    }

    // Stage 3: Fast blinking
    if (button_stage == 3) {
        bool correct = false;
        
        // Red fast blinking: press when counter shows 1
        if (led_color == 2 && game_timer % 10 == 1) {
            ESP_LOGW("ButtonPuzzle", "Stage 3 Red fast blinking: press when counter shows 1");
            correct = true;
        }
        // Blue fast blinking: press when counter shows 8
        else if (led_color == 0 && game_timer % 10 == 8) {
            ESP_LOGW("ButtonPuzzle", "Stage 3 Blue fast blinking: press when counter shows 8");
            correct = true;
        }
        // Green fast blinking: press when counter shows 9
        else if (led_color == 1 && game_timer % 10 == 9) {
            ESP_LOGW("ButtonPuzzle", "Stage 3 Green fast blinking: press when counter shows 9");
            correct = true;
        }
        // Purple fast blinking: press when counter shows 7
        else if (led_color == 3 && game_timer % 10 == 7) {
            ESP_LOGW("ButtonPuzzle", "Stage 3 Purple fast blinking: press when counter shows 7");
            correct = true;
        }
        
        if (correct) {
            // Go to stage 4 (solved)
            ESP_LOGW("ButtonPuzzle", "Stage 3 complete, puzzle solved!");
            button_stage = 4;
            puzzle_solved = true;
            call.set_rgb(0, 0, 0);  // Off
            call.set_state(false);
            call.perform();
            return;
        }

        // Wrong timing, restart
        ESP_LOGW("ButtonPuzzle", "Reset because of wrong release timing in stage 3. Color: %d, Last digit: %d", led_color, game_timer % 10);
        reset(button_stage, led_color, call);
        return;
    }

    ESP_LOGW("ButtonPuzzle", "Reset because of short press. Color: %d, Last digit: %d", led_color, game_timer % 10);

    // Wrong action, restart
    reset(button_stage, led_color, call);
}

// Handle release
void handle_release(int& button_stage, int& led_color, int game_timer, esphome::light::LightCall& call) {
    // Stage 2: Slow blinking
    if (button_stage == 2) {
        bool correct = false;
        
        // Red blinking: press when counter shows 3
        if (led_color == 2 && game_timer % 10 == 3) {
            ESP_LOGW("ButtonPuzzle", "Stage 2 Red blinking: press when counter shows 3");
            correct = true;
        }
        // Blue blinking: press when counter shows 5
        else if (led_color == 0 && game_timer % 10 == 5) {
            ESP_LOGW("ButtonPuzzle", "Stage 2 Blue blinking: press when counter shows 5");
            correct = true;
        }
        // Green blinking: press when counter shows 6
        else if (led_color == 1 && game_timer % 10 == 6) {
            ESP_LOGW("ButtonPuzzle", "Stage 2 Green blinking: press when counter shows 6");
            correct = true;
        }
        // Purple blinking: press when counter shows 2
        else if (led_color == 3 && game_timer % 10 == 2) {
            ESP_LOGW("ButtonPuzzle", "Stage 2 Purple blinking: press when counter shows 2");
            correct = true;
        }
        
        if (correct) {
            // Go to stage 3 (fast blinking)
            ESP_LOGW("ButtonPuzzle", "Stage 2 complete, advancing to stage 3");
            button_stage = 3;
            led_color = esp_random() % 4;
            if (led_color == 0) {
                call.set_effect("blink_blue_fast");
            } else if (led_color == 1) {
                call.set_effect("blink_green_fast");
            } else if (led_color == 2) {
                call.set_effect("blink_red_fast");
            } else if (led_color == 3) {
                call.set_effect("blink_purple_fast");
            }
            call.perform();
            return;
        }

        // Wrong timing, restart
        ESP_LOGW("ButtonPuzzle", "Reset because of wrong release timing in stage 2. Color: %d, Last digit: %d", led_color, game_timer % 10);
        reset(button_stage, led_color, call);
        return;
    }
}

// Handle long press (called after 3s delay, only if button still pressed)
void handle_long_press(int& button_stage, int& led_color, esphome::light::LightCall& call) {
    // Stage 1: Long press on red or purple goes to stage 2 (blinking)
    if (button_stage == 1 && (led_color == 0)) {
        //Wrong color (blue), restart
        ESP_LOGW("ButtonPuzzle", "Reset because of long press on wrong color (blue). Color: %d", led_color);
        reset(button_stage, led_color, call);
        return;
    }

    if (button_stage == 1 && (led_color == 2 || led_color == 3 || led_color == 1)) {
        button_stage = 2;
        led_color = esp_random() % 4;
        if (led_color == 0) {
            call.set_effect("blink_blue");
        } else if (led_color == 1) {
            call.set_effect("blink_green");
        } else if (led_color == 2) {
            call.set_effect("blink_red");
        } else if (led_color == 3) {
            call.set_effect("blink_purple");
        }
        call.perform();
        return;
    }
}

} // namespace ButtonPuzzle
