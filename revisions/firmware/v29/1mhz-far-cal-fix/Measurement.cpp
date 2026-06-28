#include "NearSens.h"

uint16_t measurePacket(uint8_t runType, bool *ok) {
  startMeasureAnimation();
  prepareMeasurementPower();

  uint16_t values[CAL_PINGS];
  uint8_t validCount = 0;
  uint8_t stuckCount = 0;
  uint8_t rangeCount = 0;
  uint8_t faultCount = 0;
  uint8_t noEchoCount = 0;
  uint8_t pings = (runType == RUN_CALIBRATE) ? CAL_PINGS : MEASURE_PINGS;

  for (uint8_t i = 0; i < pings; i++) {
    bool valid = false;
    uint16_t value = measureOnePing(&valid);

    if (valid && validCount < CAL_PINGS) {
      values[validCount++] = value;
    } else if (value == LCD_ECHO_STUCK) {
      stuckCount++;
    } else if (value == LCD_OUT_OF_RANGE) {
      rangeCount++;
    } else if (value == LCD_RX_FAULT) {
      faultCount++;
    } else if (value == LCD_NO_ECHO) {
      noEchoCount++;
    }

    uint32_t waitUntil = millis() + pingSpacingMs(i);
    while (!dueMs(millis(), waitUntil)) {
      serviceBackplane();
      serviceMeasureAnimation();
    }
  }

  finishMeasurementPower();
  stopMeasureAnimation();

  uint8_t minValid = (runType == RUN_CALIBRATE) ? CAL_MIN_VALID : MEASURE_MIN_VALID;
  uint8_t minCluster = (runType == RUN_CALIBRATE) ? CAL_MIN_CLUSTER : MEASURE_MIN_CLUSTER;
  uint16_t clusterMm = (runType == RUN_CALIBRATE) ? CAL_CLUSTER_MM : MEASURE_CLUSTER_MM;

  if (validCount >= minValid) {
    bool stable = false;
    uint16_t result = chooseClusterResult(values, validCount, clusterMm, minCluster, &stable);
    if (stable) {
      *ok = true;
      return result;
    }
  }

  *ok = false;
  if (stuckCount >= 2) {
    return LCD_ECHO_STUCK;
  }
  if (rangeCount >= 2) {
    return LCD_OUT_OF_RANGE;
  }
  if (faultCount >= 3) {
    return LCD_RX_FAULT;
  }
  (void)noEchoCount;
  return LCD_NO_ECHO;
}

struct EchoCandidate {
  uint32_t firstUs;
  uint32_t peakUs;
  uint16_t peakDeviation;
  uint8_t hits;
};

static bool echoCandidateLooksValid(const EchoCandidate &candidate,
                                    uint32_t txStartUs,
                                    uint16_t threshold) {
  if (candidate.peakUs < candidate.firstUs) {
    return false;
  }

  uint16_t distanceMm = timeOfFlightToMm(candidate.peakUs - txStartUs);
  if (distanceMm < echoWindowMinMm || distanceMm > echoWindowMaxMm) {
    return false;
  }

  uint32_t widthUs = candidate.peakUs - candidate.firstUs;
  if (widthUs > ECHO_MAX_PULSE_WIDTH_US) {
    return false;
  }

  bool farEcho = distanceMm >= FAR_ECHO_START_MM;

  /*
    At 1 MHz the ADC sampling loop is sparse. A weak 40-50 cm echo may cross
    the threshold for only one or two samples. Rejecting it with the same pulse
    width/hit rules as a near echo caused far calibration points to fail with
    4004/4006. Keep near echo rules stricter, but allow a stronger single-sample
    far candidate; the later packet/cluster stage still rejects random garbage.
  */
  if (farEcho) {
    if (candidate.hits >= 2) {
      return true;
    }
    return candidate.peakDeviation >= (uint16_t)(threshold + ADC_CONFIRM_MARGIN + 1U);
  }

  if (candidate.hits < ECHO_MIN_PULSE_HITS &&
      candidate.peakDeviation < (uint16_t)(threshold + ADC_STRONG_CONFIRM_MARGIN)) {
    return false;
  }
  if (widthUs < ECHO_MIN_PULSE_WIDTH_US && candidate.hits <= ECHO_MIN_PULSE_HITS) {
    return false;
  }

  return true;
}

static void storeEchoCandidate(EchoCandidate *candidates,
                               uint8_t *candidateCount,
                               uint32_t txStartUs,
                               uint32_t firstUs,
                               uint32_t peakUs,
                               uint16_t peakDeviation,
                               uint8_t hits,
                               uint16_t nearThreshold,
                               uint16_t farThreshold) {

  EchoCandidate candidate;
  candidate.firstUs = firstUs;
  candidate.peakUs = peakUs;
  candidate.peakDeviation = peakDeviation;
  candidate.hits = hits;

  uint16_t candidateThreshold = nearThreshold;
  if (timeOfFlightToMm(candidate.peakUs - txStartUs) >= FAR_ECHO_START_MM) {
    candidateThreshold = farThreshold;
  }

  if (!echoCandidateLooksValid(candidate, txStartUs, candidateThreshold)) {
    return;
  }

  if (*candidateCount < ECHO_CANDIDATE_MAX) {
    candidates[*candidateCount] = candidate;
    (*candidateCount)++;
    return;
  }

  uint8_t weakestIndex = 0;
  for (uint8_t i = 1; i < ECHO_CANDIDATE_MAX; i++) {
    if (candidates[i].peakDeviation < candidates[weakestIndex].peakDeviation) {
      weakestIndex = i;
    }
  }

  if (candidate.peakDeviation > candidates[weakestIndex].peakDeviation) {
    candidates[weakestIndex] = candidate;
  }
}

static bool chooseBestEchoCandidate(EchoCandidate *candidates,
                                    uint8_t candidateCount,
                                    uint32_t txStartUs,
                                    uint16_t *distanceMm) {
  if (candidateCount == 0 || distanceMm == 0) {
    return false;
  }

  uint8_t bestIndex = 0;
  for (uint8_t i = 1; i < candidateCount; i++) {
    bool better = false;

    if (candidates[i].peakDeviation >
        (uint16_t)(candidates[bestIndex].peakDeviation + ECHO_PEAK_TIE_MARGIN)) {
      better = true;
    } else if ((uint16_t)(candidates[bestIndex].peakDeviation + ECHO_PEAK_TIE_MARGIN) >=
                 candidates[i].peakDeviation &&
               (uint16_t)(candidates[i].peakDeviation + ECHO_PEAK_TIE_MARGIN) >=
                 candidates[bestIndex].peakDeviation) {
      // Similar strength: take the earlier echo, because the meter is short range.
      if (candidates[i].peakUs < candidates[bestIndex].peakUs) {
        better = true;
      }
    }

    if (better) {
      bestIndex = i;
    }
  }

  *distanceMm = timeOfFlightToMm(candidates[bestIndex].peakUs - txStartUs);
  return true;
}

uint16_t measureOnePing(bool *valid) {
  *valid = false;

  uint16_t minAdc = 1023;
  uint16_t maxAdc = 0;
  uint16_t baseline = captureBaseline(&minAdc, &maxAdc);
  uint16_t noise = maxAdc - minAdc;

  if (baseline <= ADC_RAIL_LOW || baseline >= ADC_RAIL_HIGH ||
      noise > ADC_NOISE_FAULT) {
    return LCD_RX_FAULT;
  }

  uint16_t threshold = (uint16_t)(noise * 2U + ADC_THRESHOLD_EXTRA);
  if (threshold < ADC_THRESHOLD_MIN) {
    threshold = ADC_THRESHOLD_MIN;
  }

  uint16_t farThreshold = (uint16_t)(noise + ADC_FAR_THRESHOLD_EXTRA);
  if (farThreshold < ADC_FAR_THRESHOLD_MIN) {
    farThreshold = ADC_FAR_THRESHOLD_MIN;
  }
  if (farThreshold > threshold) {
    farThreshold = threshold;
  }

  uint32_t txStartUs = micros();
  sendBurst();
  uint32_t txEndUs = micros();

  uint32_t scanStartUs = txEndUs + ECHO_GUARD_AFTER_TX_US;
  uint32_t scanEndUs = txStartUs + ECHO_TIMEOUT_FROM_TX_START_US;

  // Keep the critical RX window digitally quiet. Missing one LCD BP edge for a
  // few milliseconds is less harmful than toggling LCD pins while sampling echo.
  while (!dueUs(micros(), scanStartUs)) {
  }

  EchoCandidate candidates[ECHO_CANDIDATE_MAX];
  uint8_t candidateCount = 0;

  bool inPulse = false;
  uint32_t firstUs = 0;
  uint32_t peakUs = 0;
  uint16_t peakDeviation = 0;
  uint8_t hits = 0;

  while (true) {
    uint32_t sampleUs = micros();
    if (dueUs(sampleUs, scanEndUs)) {
      break;
    }

    uint16_t adcValue = readEchoAdc();
    uint16_t deviation = absDiffAdc(adcValue, baseline);
    uint16_t activeThreshold = threshold;
    if (timeOfFlightToMm(sampleUs - txStartUs) >= FAR_ECHO_START_MM) {
      activeThreshold = farThreshold;
    }

    if (deviation >= activeThreshold) {
      if (!inPulse) {
        inPulse = true;
        firstUs = sampleUs;
        peakUs = sampleUs;
        peakDeviation = deviation;
        hits = 1;
      } else {
        if (hits < 255) {
          hits++;
        }
        if (deviation > peakDeviation) {
          peakDeviation = deviation;
          peakUs = sampleUs;
        }
      }
      continue;
    }

    if (inPulse) {
      storeEchoCandidate(candidates, &candidateCount, txStartUs,
                         firstUs, peakUs, peakDeviation, hits,
                         threshold, farThreshold);
      inPulse = false;
      firstUs = 0;
      peakUs = 0;
      peakDeviation = 0;
      hits = 0;
    }
  }

  if (inPulse) {
    storeEchoCandidate(candidates, &candidateCount, txStartUs,
                       firstUs, peakUs, peakDeviation, hits,
                       threshold, farThreshold);
  }

  uint16_t distanceMm = 0;
  if (chooseBestEchoCandidate(candidates, candidateCount, txStartUs, &distanceMm)) {
    *valid = true;
    return distanceMm;
  }

  return LCD_NO_ECHO;
}

uint8_t pingSpacingMs(uint8_t pingIndex) {
  static const uint8_t offsets[4] = {0, 3, 1, 5};
  return (uint8_t)(INTER_PING_MS + offsets[pingIndex & 0x03]);
}

uint16_t captureBaseline(uint16_t *minAdc, uint16_t *maxAdc) {
  uint32_t sum = 0;
  *minAdc = 1023;
  *maxAdc = 0;

  serviceBackplane();
  for (uint16_t i = 0; i < BASELINE_SAMPLES; i++) {
    uint16_t adcValue = readEchoAdc();
    sum += adcValue;
    if (adcValue < *minAdc) {
      *minAdc = adcValue;
    }
    if (adcValue > *maxAdc) {
      *maxAdc = adcValue;
    }
    delayMicroseconds(10);
  }

  return (uint16_t)((sum + (BASELINE_SAMPLES / 2)) / BASELINE_SAMPLES);
}

uint16_t chooseClusterResult(uint16_t *values, uint8_t count, uint16_t clusterMm, uint8_t minCluster, bool *stable) {
  sortValues(values, count);
  *stable = false;

  /*
    Practical rule for this hardware: use the first stable distance cluster,
    not the largest one. A later false lobe can be very repeatable, and the
    older "largest cluster wins" rule could turn a real 250 mm target into a
    stable-looking 300 mm reading. The nearest stable echo is the safer choice
    for a short range 10-50 cm distance meter.
  */
  for (uint8_t start = 0; start < count; start++) {
    uint8_t end = start;
    while ((uint8_t)(end + 1) < count &&
           (uint16_t)(values[end + 1] - values[start]) <= clusterMm) {
      end++;
    }

    uint8_t members = end - start + 1;
    if (members >= minCluster) {
      *stable = true;
      uint8_t middle = start + (members / 2);
      if ((members & 0x01) != 0) {
        return values[middle];
      }
      return (uint16_t)(((uint32_t)values[middle - 1] + values[middle] + 1UL) / 2UL);
    }
  }

  return values[count / 2];
}

void sortValues(uint16_t *values, uint8_t count) {
  for (uint8_t i = 1; i < count; i++) {
    uint16_t key = values[i];
    int8_t j = (int8_t)i - 1;
    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      j--;
    }
    values[j + 1] = key;
  }
}
