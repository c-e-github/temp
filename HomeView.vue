<template>
    <BasePage :totalData="liveData?.total" :title="$t('home.LiveData')" :isLoading="dataLoading" :isWideScreen="true">
        <InverterTotalInfo :totalData="liveData?.total" /><br />
        <div class="row gy-3">

<!-- ce-change: immer volle breite durch col-sm-12 col-md-12
            <div class="tab-content" id="v-pills-tabContent" :class="{
                'col-sm-9 col-md-10': inverterData.length > 1,
                'col-sm-12 col-md-12': inverterData.length == 1
            }">
-->
            <div class="tab-content col-sm-12 col-md-12" id="v-pills-tabContent">

<!-- ce-change: <div v-for="inverter in inverterData" :key="inverter.serial" class="tab-pane fade show"
geändert zu     <div v-if="inverterData.length > 0" :id="'v-pills-' + inverter.serial" 
dadurch wird das div immer angezeigt, und nur ein inverter -->

                <div v-if="inverterData.length > 0" :id="'v-pills-' + inverter.serial" 
                    role="tabpanel" :aria-labelledby="'v-pills-' + inverter.serial + '-tab'" tabindex="0">

                    <div class="card">
                        <!--<div class="card-header d-flex justify-content-between align-items-start"-->
                        <div class="card-header d-flex d-flex flex-column align-items-stretch"
                            :class="{
                                'text-bg-tertiary': !inverter.poll_enabled,
                                'text-bg-danger': inverter.poll_enabled && !inverter.reachable,
                                'text-bg-warning': inverter.poll_enabled && inverter.reachable && !inverter.producing,
                                'card-header text-bg-success': inverter.poll_enabled && inverter.reachable && inverter.producing,
                            }">
        <!-- ce-change: oben statt "text-bg-primary" "card-header text-bg-success" eingesetzt, um hintergrund grün zu machen
        'text-bg-primary': inverter.poll_enabled && inverter.reachable && inverter.producing, -->

                           <div class="p-1 flex-grow-1 d-flex flex-wrap" style="min-width:230px;">

                               <!-- ce-change: Anzeige Wechselrichter Name  -->
                               <div style="padding-right: 2em;">
                                   {{ inverter.name }}
                               </div>

                               <!-- ce-change: Anzeige Wechselrichter Temperatur  -->
                               <div style="padding-right: 2em;">
                                   {{$t('inverterchannelproperty.Temperature')}}:
                                   {{ $n(liveData?.["inverters"]?.[0]?.["INV"]?.["0"]?.["Temperature"]?.v ?? 0, 'decimal', {
                                       minimumFractionDigits: liveData?.["inverters"]?.[0]?.["INV"]?.["0"]?.["Temperature"]?.d ?? '1',
                                       maximumFractionDigits: liveData?.["inverters"]?.[0]?.["INV"]?.["0"]?.["Temperature"]?.d ?? '1'})
                                   }}
                                   {{ liveData?.["inverters"]?.[0]?.["INV"]?.["0"]?.["Temperature"]?.u ?? '°C' }}
                               </div>

                               <div style="padding-right: 2em; font-size: 0.875em;">
                                   <span style="vertical-align: middle;">
                                       ({{ $t('home.SerialNumber') }}{{ inverter.serial }})
                                   </span>

                               </div>
                               <div style="padding-right: 2em;">
                                   {{ $t('home.CurrentLimit') }}<template v-if="inverter.limit_absolute > -1"> {{
                                           $n(inverter.limit_absolute, 'decimalNoDigits')
                                   }} W | </template>{{ $n(inverter.limit_relative / 100, 'percent') }}
                               </div>

                               <div style="padding-right: 2em;">
                                   {{ $t('home.DataAge') }} {{ $t('home.Seconds', {'val': $n(inverter.data_age) }) }}
                                   <template v-if="inverter.data_age > 300">
                                       / {{ calculateAbsoluteTime(inverter.data_age) }}
                                    </template>
                               </div>
                           </div>

                           <div class="btn-toolbar flex-wrap" role="toolbar">

                                <div class="btn-group me-2" role="group">
                                   <div class="aura-div" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                      <button :disabled="!isLogged" type="button" class="btn btn-sm btn-info"
                                         @click="onShowPowerSettings(inverter.serial)" v-tooltip :title="$t('home.TurnOnOff')">
                                         <BIconPower style="font-size:24px;" />
                                      </button>
                                   </div>
                                </div>

                                <div class="btn-group me-2" role="group">
                                   <div class="aura-div" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                      <button :disabled="!isLogged" type="button" class="btn btn-sm btn-info"
                                         @click="onShowLimitSettings(inverter.serial)" v-tooltip :title="$t('home.ShowSetInverterLimit')">
                                         <BIconSpeedometer style="font-size:24px;" />
                                      </button>
                                   </div>
                                </div>

                                <div class="btn-group me-2" role="group">
                                   <div class="aura-div" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                      <button type="button" class="btn btn-sm btn-info"
                                         @click="onShowDevInfo(inverter.serial)" v-tooltip :title="$t('home.ShowInverterInfo')">
                                         <BIconCpu style="font-size:24px;" />
                                     </button>
                                   </div>
                                </div>

                                <div class="btn-group me-2" role="group">
                                   <div class="aura-div" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                      <button type="button" class="btn btn-sm btn-info"
                                         @click="onShowGridProfile(inverter.serial)" v-tooltip :title="$t('home.ShowGridProfile')">
                                         <!--    <b-icon-house stacked></b-icon-house> -->
                                         <BIconLightning style="font-size:24px;" />
                                     </button>
                                   </div>
                                </div>

                                <!-- ce-change: button zum display ein-/ausschalten           -->
                                <div class="btn-group" role="group" style="margin-right: 0.5rem" >
                                   <div id="auraD" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                       <button type="button" class="btn btn-sm btn-info" id="displaybutton"
                                          @click="ontoggleDisplay()" v-tooltip :title="displayTooltip"> 
                                          <BIconDisplay style="font-size:24px;" />
                                       </button>
                                   </div>
                                </div>

                                <!-- ce-change: button zum lora-funk ein-/ausschalten           -->
                                <div class="btn-group" role="group" style="margin-right: 0.5rem">
                                   <div id="auraL" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                       <button type="button" class="btn btn-sm btn-info" id="lorabutton"
                                          @click="ontoggleLora()" v-tooltip :title="loraTooltip"> 
                                          <BIconBroadcastPin style="font-size:24px;" />
                                       </button>
                                   </div>
                                </div>

                                <div class="btn-group" role="group">
                                   <div class="aura-div" style="padding: 0.5rem; background: radial-gradient(circle at center, #198754, #198754 80%); border-radius: 10%;">
                                      <button v-if="inverter.events >= 0" type="button"
                                         class="btn btn-sm btn-info position-relative"
                                         @click="onShowEventlog(inverter.serial)" v-tooltip :title="$t('home.ShowEventlog')">
                                         <BIconJournalText style="font-size:24px;" />
                                         <span
                                            class="position-absolute top-0 start-100 translate-middle badge rounded-pill text-bg-danger">
                                            {{ inverter.events }}
                                            <span class="visually-hidden">{{ $t('home.UnreadMessages') }}</span>
                                         </span>
                                      </button>
                                   </div>
                                </div>
                            </div>
                        </div>

                        <div class="card-body">
                            <div class="row flex-row-reverse flex-wrap-reverse g-3">
                                <template v-for="chanType in [{obj: inverter.INV, name: 'INV'}, {obj: inverter.AC, name: 'AC'}, {obj: inverter.DC, name: 'DC'}].reverse()">
                                    <template v-for="channel in Object.keys(chanType.obj ?? {}).sort().reverse().map(x=>+x)" :key="channel">
                                        <!-- ce-change: in nächste zeile v-if="chanType.name != 'INV'" eingefügt, um
                                        anzeige allgemein zu entfernen -->
                                        <template v-if="chanType.name != 'INV'">
                                            <div class="col">
                                                <InverterChannelInfo :channelData="chanType.obj[channel]"
                                                    :channelType="chanType.name"
                                                    :channelNumber="channel" />
                                            </div>
                                        </template>
                                    </template>
                                </template>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </BasePage>


    <div class="modal" id="eventView" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('home.EventLog') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="text-center" v-if="eventLogLoading">
                        <div class="spinner-border" role="status">
                            <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                        </div>
                    </div>

                    <EventLog v-if="!eventLogLoading" :eventLogList="eventLogList" />
                </div>

                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onHideEventlog"
                        data-bs-dismiss="modal">{{ $t('home.Close') }}</button>
                </div>

            </div>
        </div>
    </div>

    <div class="modal" id="devInfoView" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('home.InverterInfo') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="text-center" v-if="devInfoLoading">
                        <div class="spinner-border" role="status">
                            <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                        </div>
                    </div>

                    <DevInfo v-if="!devInfoLoading" :devInfoList="devInfoList" />
                </div>

                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onHideDevInfo"
                        data-bs-dismiss="modal">{{ $t('home.Close') }}</button>
                </div>
            </div>
        </div>
    </div>

    <div class="modal" id="gridProfileView" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('home.GridProfile') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="text-center" v-if="gridProfileLoading">
                        <div class="spinner-border" role="status">
                            <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                        </div>
                    </div>

                    <GridProfile v-if="!gridProfileLoading" :gridProfileList="gridProfileList" :gridProfileRawList="gridProfileRawList" />
                </div>

                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="onHideGridProfile"
                        data-bs-dismiss="modal">{{ $t('home.Close') }}</button>
                </div>
            </div>
        </div>
    </div>

    <div class="modal" id="limitSettingView" ref="limitSettingView" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <form @submit="onSubmitLimit">
                    <div class="modal-footer justify-content-between align-items-center">
                        <h5 class="modal-title">{{ $t('home.LimitSettings') }}</h5>
                        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                    </div>
                    <div class="modal-body">

                        <BootstrapAlert v-model="showAlertLimit" :variant="alertTypeLimit">
                            {{ alertMessageLimit }}
                        </BootstrapAlert>
                        <div class="text-center" v-if="limitSettingLoading">
                            <div class="spinner-border" role="status">
                                <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                            </div>
                        </div>

                        <template v-if="!limitSettingLoading">

                            <div class="row mb-3" style="margin-top:-15px;">
                                <label for="inputCurrentLimit" class="col-sm-3 col-form-label">{{ $t('home.CurrentLimit') }} </label>
                                <div class="col-sm-4">
                                    <div class="input-group">
                                        <input type="text" class="form-control" id="inputCurrentLimit"
                                            aria-describedby="currentLimitType" v-model="currentLimitRelative"
                                            disabled />
                                        <span class="input-group-text" id="currentLimitType">%</span>
                                    </div>
                                </div>
                                <div class="col-sm-auto d-flex align-items-center" v-if="currentLimitList.max_power > 0">
                                     <span>=</span>
                                </div>
                                <div class="col-sm-4" v-if="currentLimitList.max_power > 0">
                                    <div class="input-group">
                                        <input type="text" class="form-control" id="inputCurrentLimitAbsolute"
                                            aria-describedby="currentLimitTypeAbsolute" v-model="currentLimitAbsolute"
                                            disabled />
                                        <span class="input-group-text" id="currentLimitTypeAbsolute">W</span>
                                    </div>
                                </div>
                            </div>

                            <div class="d-flex align-items-center gap-2 mb-3 flex-nowrap">
                                <label for="inputLastLimitSet" class="mb-0"> {{ $t('home.LastLimitSetStatus') }} </label>
                                <span class="badge" :class="{
                                    'text-bg-danger': currentLimitList.limit_set_status == 'Failure',
                                    'text-bg-warning': currentLimitList.limit_set_status == 'Pending',
                                    'text-bg-success': currentLimitList.limit_set_status == 'Ok',
                                    'text-bg-secondary': currentLimitList.limit_set_status == 'Unknown',
                                }">
                                    {{ $t('home.' + currentLimitList.limit_set_status) }}
                                </span>
                            </div>

                            <div class="row align-items-center gx-2 gy-1">
                                <!-- Label -->
                                <div class="col-12 col-md-auto">
                                    <label for="inputTargetLimit" class="col-form-label">{{ $t('home.SetLimit') }}</label>
                                </div>
                                <div class="col-12 col-md-auto">
                                    <input type="number" name="inputTargetLimit" class="form-control"
                                        id="inputTargetLimit" :min="targetLimitMin" :max="targetLimitMax"
                                        v-model="targetLimitList.limit_value">
                                </div>
<!--                                    <button class="btn btn-primary dropdown-toggle" type="button"
                                            data-bs-toggle="dropdown" aria-expanded="false">{{ targetLimitTypeText
                                            }}</button>
                                        <ul class="dropdown-menu dropdown-menu-end">
                                            <li><a class="dropdown-item" @click="onSelectType(1)" href="#">{{ $t('home.Relative') }}</a></li>
                                            <li><a class="dropdown-item" @click="onSelectType(0)" href="#">{{ $t('home.Absolute') }}</a></li>
                                        </ul>
-->

                                <div class="col-12 col-md-auto">
                                    <span>  {{ $t('home.Relative') }}</span>  
                                </div>
                                    
                                <div v-if="targetLimitType == 0" class="alert alert-secondary mt-3" role="alert" v-html="$t('home.LimitHint')"></div>
                            </div>
                            <div>
                                <span> {{ $t('home.ManualHint') }} </span>
                            </div>
                        </template>
                        <div class="d-flex justify-content-evenly" style="margin-top:10px;">
                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(false)">
                                {{ $t('home.SetNonPersistent') }}
                            </button>
                            <button type="submit" class="btn btn-danger" @click="onSetLimitSettings(true)">
                                {{ $t('home.SetPersistent') }}
                            </button>
                        </div>
                    </div>




                    <div class="modal-footer flex-column align-items-stretch">
                        <h5 class="modal-title">{{ $t('home.LimitSettingsAuto') }}</h5>
                        <div class="mb-3 text-start">
                            {{ $t('home.AutoHint') }}
                            <!-- Responsive Eingabezeile -->
                            <div class="row align-items-center gx-2 gy-1">
                                <!-- Label -->
                                <div class="col-12 col-md-auto">
                                    <label for="inputAutoLimit" class="col-form-label">
                                        {{ $t('home.NotMore') }}  
                                    </label>
                                </div>
                                <!-- Input -->
                                <div class="col-12 col-md-auto">
                                    <input
                                        type="number"
                                        name="inputAutoLimit"
                                        class="form-control"
                                        :class="{'is-invalid': autoLimitInvalid}"
                                        id="inputAutoLimit"
                                        :min="targetAutoMin"
                                        :max="targetAutoMax"
                                        v-model.number="targetLimitList.autolimit_value"
                                        @change="checkAutoLimit">
                                    <div class="invalid-feedback">
                                        {{ $t('home.Invalidmaxleistung') }}  
                                    </div>
                                </div>
                                <!-- Zusatztext -->
                                <div class="col-12 col-md-auto">
                                    <span>  {{ $t('home.Relative') }}</span>  
                                </div>
                            </div>
                            <!-- Button -->
                            <div class="mt-1 text-center">
                                <button
                                    class="btn btn-danger px-4"
                                    @click="onSetmaxleistung()">
                                    {{ $t('home.Save') }}
                                </button>
                            </div>
                        </div>
                    </div>




                    <div class="modal-footer" style="margin-top:-15px;">
                        <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t('home.Close') }}</button>
                    </div>
                </form>
            </div>
        </div>
    </div>

    <div class="modal" id="powerSettingView" ref="powerSettingView" tabindex="-1">
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title">{{ $t('home.PowerSettings') }}</h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">

                    <BootstrapAlert v-model="showAlertPower" :variant="alertTypePower">
                        {{ alertMessagePower }}
                    </BootstrapAlert>
                    <div class="text-center" v-if="powerSettingLoading">
                        <div class="spinner-border" role="status">
                            <span class="visually-hidden">{{ $t('home.Loading') }}</span>
                        </div>
                    </div>

                    <template v-if="!powerSettingLoading">
                        <div class="row mb-3 align-items-center">
                            <label for="inputLastPowerSet" class="col col-form-label">{{ $t('home.LastPowerSetStatus') }}</label>
                            <div class="col">
                                <span class="badge" :class="{
                                    'text-bg-danger': successCommandPower == 'Failure',
                                    'text-bg-warning': successCommandPower == 'Pending',
                                    'text-bg-success': successCommandPower == 'Ok',
                                    'text-bg-secondary': successCommandPower == 'Unknown',
                                }">
                                    {{ $t('home.' + successCommandPower) }}
                                </span>
                            </div>
                        </div>

                        <div class="d-grid gap-2 col-6 mx-auto">
                            <button type="button" class="btn btn-success" @click="onSetPowerSettings(true)">
                                <BIconToggleOn class="fs-4" />&nbsp;{{ $t('home.TurnOn') }}
                            </button>
                            <button type="button" class="btn btn-danger" @click="onSetPowerSettings(false)">
                                <BIconToggleOff class="fs-4" />&nbsp;{{ $t('home.TurnOff') }}
                            </button>
                            <button type="button" class="btn btn-warning" @click="onSetPowerSettings(true, true)">
                                <BIconArrowCounterclockwise class="fs-4" />&nbsp;{{ $t('home.Restart') }}
                            </button>
                        </div>
                    </template>

                </div>

                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t('home.Close') }}</button>
                </div>
            </div>
        </div>
    </div>
</template>




<script lang="ts">
//####################################################################################

//  liveData: {} as LiveData, wieder aktiv setzen, wenn testmodus: boolean = false;

let testmodus: boolean = true;
//####################################################################################
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import DevInfo from '@/components/DevInfo.vue';
import EventLog from '@/components/EventLog.vue';
import GridProfile from '@/components/GridProfile.vue';
import HintView from '@/components/HintView.vue';
import InverterChannelInfo from "@/components/InverterChannelInfo.vue";
import InverterTotalInfo from '@/components/InverterTotalInfo.vue';
import type { DevInfoStatus } from '@/types/DevInfoStatus';
import type { EventlogItems } from '@/types/EventlogStatus';
import type { GridProfileStatus } from '@/types/GridProfileStatus';
import type { GridProfileRawdata } from '@/types/GridProfileRawdata';
import type { LimitConfig } from '@/types/LimitConfig';
import type { LimitStatus } from '@/types/LimitStatus';
import type { Inverter, LiveData } from '@/types/LiveDataStatus';
import { authHeader, authUrl, handleResponse, isLoggedIn } from '@/utils/authentication';
import * as bootstrap from 'bootstrap';

import {
    BIconArrowCounterclockwise,
    BIconCheckCircleFill,
    BIconCpu,
    BIconExclamationCircleFill,
    BIconJournalText,
    BIconOutlet,
    BIconPower,
    BIconDisplay,
    BIconHouse,
    BIconLightning,
    BIconBroadcastPin,
    BIconSpeedometer,
    BIconToggleOff,
    BIconToggleOn,
    BIconXCircleFill
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

let auracolor = "";
let aurashinecolor = "";

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        DevInfo,
        EventLog,
        GridProfile,
        HintView,
        InverterChannelInfo,
        InverterTotalInfo,
        BIconArrowCounterclockwise,
        BIconCheckCircleFill,
        BIconCpu,
        BIconExclamationCircleFill,
        BIconJournalText,
        BIconOutlet,
        BIconPower,
        BIconDisplay,
        BIconHouse,
        BIconLightning,
        BIconBroadcastPin,
        BIconSpeedometer,
        BIconToggleOff,
        BIconToggleOn,
        BIconXCircleFill,
    },
    data() {
        return {
            isLogged: this.isLoggedIn(),
            socket: {} as WebSocket,
            heartInterval: 0,
            dataAgeInterval: 0,
            //####################################################################################
            dataLoading: !testmodus, // Sets to `false` if `testmodus` is `true`, otherwise `true`
            //liveData: {} as LiveData,
            //####################################################################################
liveData: {
    inverters: [{
        serial: 11617160,
        name: "Anlage 1",
        order: 1,
        data_age: 1,
        poll_enabled: true,
        reachable: true,
        producing: true,
        limit_relative: 20,
        limit_absolute: -1,
        AC: {
            "0": {
                name: { u: "Phase 1" },  // ← Korrigiert
                Power: { v: 150, u: "W", d: 1 },
                Voltage: { v: 230, u: "V", d: 1 },
                Current: { v: 0.65, u: "A", d: 2 },
                "Power DC": { v: 155, u: "W", d: 1 },
                YieldDay: { v: 230, u: "Wh", d: 0 },
                YieldTotal: { v: 26.1, u: "kWh", d: 3 },
                Frequency: { v: 50.0, u: "Hz", d: 2 },
                PowerFactor: { v: 1.0, u: "", d: 3 },
                ReactivePower: { v: 0, u: "var", d: 1 },
                Efficiency: { v: 96.8, u: "%", d: 3 }
            }
        },
        DC: {
            "0": {
                name: { u: "String 1" },
                Power: { v: 155, u: "W", d: 1 },
                Voltage: { v: 48.5, u: "V", d: 1 },
                Current: { v: 3.2, u: "A", d: 2 },
                YieldDay: { v: 230, u: "Wh", d: 0 },
                YieldTotal: { v: 26.1, u: "kWh", d: 3 },
                Irradiation: { v: 0, u: "%", d: 3 }
            },
            "1": {
                name: { u: "String 1" },
                Power: { v: 154, u: "W", d: 1 },
                Voltage: { v: 48.4, u: "V", d: 1 },
                Current: { v: 3.2, u: "A", d: 2 },
                YieldDay: { v: 229, u: "Wh", d: 0 },
                YieldTotal: { v: 26.0, u: "kWh", d: 3 },
                Irradiation: { v: 0, u: "%", d: 3 }
            }
        },
        INV: {
            "0": {
                Temperature: { v: 33, u: "°C", d: 1 }
            }
        },
        events: 0
    }],
    total: {
        Power: { v: 150, u: "W", d: 0 },
        YieldDay: { v: 0.23, u: "kWh", d: 3 },
        YieldTotal: { v: 26.1, u: "V", d: 1 },
        YieldYesterDay: { v: 1.370, u: "kWh", d: 3 },
        DisplayOff: false,
        LoraOn: true,
        Stromzaehler: { v: 370, u: "W", d: 3 }
    },
    hints: {
        time_sync: false,
        radio_problem: false,
        default_password: false
    }
} as any as LiveData,


            //####################################################################################
            isFirstFetchAfterConnect: true,
            eventLogView: {} as bootstrap.Modal,
            eventLogList: {} as EventlogItems,
            eventLogLoading: true,
            devInfoView: {} as bootstrap.Modal,
            devInfoList: {} as DevInfoStatus,
            devInfoLoading: true,
            gridProfileView: {} as bootstrap.Modal,
            gridProfileList: {} as GridProfileStatus,
            gridProfileRawList: {} as GridProfileRawdata,
            gridProfileLoading: true,

            limitSettingView: {} as bootstrap.Modal,
            limitSettingLoading: true,

            currentLimitList: {} as LimitStatus,
            targetLimitList: {} as LimitConfig,

            targetLimitMin: 0,
            targetAutoMin: 10,
            targetLimitMax: 100,
            targetAutoMax: 100,
            targetLimitTypeText: this.$t('home.Relative'),
            targetLimitType: 1,
            targetLimitPersistent: false,

            alertMessageLimit: "",
            alertTypeLimit: "info",
            showAlertLimit: false,

            powerSettingView: {} as bootstrap.Modal,
            powerSettingSerial: 0,
            powerSettingLoading: true,
            alertMessagePower: "",
            alertTypePower: "info",
            showAlertPower: false,
            successCommandPower: "",
        };
    },

    provide() {
        // Wir übergeben das reaktive Objekt direkt. 
        // Da liveData im data() Block definiert ist, IST es bereits reaktiv.
        return {
            liveData: this.liveData 
        };
    },

    created() {
        this.getInitialData();
        this.initSocket();
        this.initDataAgeing();
        this.$emitter.on("logged-in", () => {
            this.isLogged = this.isLoggedIn();
        });
        this.$emitter.on("logged-out", () => {
            this.isLogged = this.isLoggedIn();
        });
    },

    mounted() {
        this.eventLogView = new bootstrap.Modal('#eventView');
        this.devInfoView = new bootstrap.Modal('#devInfoView');
        this.gridProfileView = new bootstrap.Modal('#gridProfileView');
        this.limitSettingView = new bootstrap.Modal('#limitSettingView');
        this.powerSettingView = new bootstrap.Modal('#powerSettingView');

        (this.$refs.limitSettingView as HTMLElement).addEventListener("hide.bs.modal", this.onHideLimitSettings);
        (this.$refs.powerSettingView as HTMLElement).addEventListener("hide.bs.modal", this.onHidePowerSettings);
    },

    unmounted() {
        this.closeSocket();
    },

   updated() {
   //################### loarabutton und displaybutton farbe und aura setzen nach start/reload #############################
      this.updatedisplaybutton();
      this.updatelorabutton();
      //#####################################
       // Select first tab
       if (this.isFirstFetchAfterConnect) {
           this.isFirstFetchAfterConnect = false;
           const firstTabEl = document.querySelector(
               "#v-pills-tab:first-child button"
           );
           if (firstTabEl != null) {
               const firstTab = new bootstrap.Tab(firstTabEl);
               firstTab.show();
           }
       }
    },

    computed: {
        //###################### damit nur ein inverter angezeigt wird ######################
        inverter() {
            return this.inverterData[0];
        },

        autoLimitInvalid() {
            const val = this.targetLimitList.autolimit_value;
            return val < this.targetAutoMin || val > this.targetAutoMax;
        },

       //      firstInverter() {
       //       this.updatelorabutton();
       //     this.updatedisplaybutton(); 
       //   return this.inverterData.length > 0 ? [this.inverterData[0]] : [];
       //},
       //#################################################################################
      displayTooltip() {
          return this.liveData.total?.DisplayOff 
          ? this.$t('home.toggleDisplayOn') 
          : this.$t('home.toggleDisplayOff');
      },
      loraTooltip() {
          return this.liveData.total?.LoraOn 
          ? this.$t('home.toggleLoraOff') 
          : this.$t('home.toggleLoraOn');
      },

      currentLimitAbsolute(): string {
          if (this.currentLimitList.max_power > 0) {
              return this.$n(this.currentLimitList.limit_relative * this.currentLimitList.max_power / 100,
                  'decimalTwoDigits');
          }
          return "0";
      },
      currentLimitRelative(): string {
          return this.$n(this.currentLimitList.limit_relative,
              'decimalTwoDigits');
      },
  
      //es gibt nur eine Inverter, daher code vereinfacht
      inverterData(): Inverter[] {
          return this.liveData.inverters;
      }
   },


    methods: {

        checkAutoLimit() {
            const val = this.targetLimitList.autolimit_value;
            if (val < this.targetAutoMin) this.targetLimitList.autolimit_value = this.targetAutoMin;
            if (val > this.targetAutoMax) this.targetLimitList.autolimit_value = this.targetAutoMax;
        },

      updatedisplaybutton(){
         if (this.liveData.inverters[0].producing){
            auracolor = "#198754";
            aurashinecolor = "yellow";
            document.querySelectorAll('.aura-div').forEach(div => {(div as HTMLElement).style.background = "#198754";});
         } else {
            auracolor = "#ffc107";
            aurashinecolor = "white";
            document.querySelectorAll('.aura-div').forEach(div => {(div as HTMLElement).style.background = "#ffc107";});
         }

         let temp = document.getElementById("displaybutton");
         if (temp){
            if (this.liveData.total.DisplayOff){
               temp.style.backgroundColor = "#0dcaf0";
               temp = document.getElementById("auraD");
               if (temp){
                  temp.style.background = `radial-gradient(circle at center, ${auracolor}, ${auracolor} 80%)`;
               }
            } else {
               temp.style.backgroundColor='lightgreen';
               temp = document.getElementById("auraD");
               if (temp){
                  temp.style.background = `radial-gradient(circle at center, ${aurashinecolor}, ${auracolor} 80%)`;
               }
            }
         }
      },
      updatelorabutton(){
         if (this.liveData.inverters[0].producing){
            auracolor = "#198754";
            aurashinecolor = "yellow";
            document.querySelectorAll('.aura-div').forEach(div => {(div as HTMLElement).style.background = "#198754";});
         } else {
            auracolor = "#ffc107";
            aurashinecolor = "white";
            document.querySelectorAll('.aura-div').forEach(div => {(div as HTMLElement).style.background = "#ffc107";});
         }

         let ma = document.getElementById("modusanzeige");
         let temp = document.getElementById("lorabutton");
         if (this.liveData.total.LoraOn){
            if (ma){
               ma.innerText = this.$t('home.ModusA');
               ma.style.color="lightgreen";
            } 
            if (temp) {
                temp.style.backgroundColor='lightgreen';
                temp = document.getElementById("auraL");
                if (temp){
                    temp.style.background = `radial-gradient(circle at center, ${aurashinecolor}, ${auracolor} 80%)`;
                }
            }
            //console.log("lora ist an");
         } else{
            if (ma) {
                ma.innerText = this.$t('home.ModusM');
                ma.style.color="red";
            }
            if (temp) {
                temp.style.backgroundColor = "#0dcaf0";
                temp = document.getElementById("auraL");
                if (temp){
                    temp.style.background = `radial-gradient(circle at center, ${auracolor}, ${auracolor} 80%)`;
                }
            }
            // console.log("lora ist aus");
         }
      },

      isLoggedIn,

      getInitialData() {
         if (testmodus == true){  // damit im testmodus keine webserver-abfrage gemacht wird
            this.dataLoading = false;
            return;
         };
         this.dataLoading = true;
         fetch("/api/livedata/status", { headers: authHeader() })
         .then((response) => handleResponse(response, this.$emitter, this.$router))
         .then((data) => {
            this.liveData = data;
            this.dataLoading = false;
         });
      },

        initSocket() {
            if (testmodus == true){  // damit im testmodus keine webserver-abfrage gemacht wird
               return;
            };
            console.log("Starting connection to WebSocket Server");
            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === "https:" ? "wss" : "ws"
                }://${authString}${host}/livedata`;
            this.socket = new WebSocket(webSocketUrl);

            this.socket.onmessage = (event) => {

//##################################################################################
               //console.log(event); 
//##################################################################################
               if (event.data != "{}") {
                  //console.log(event.data); 
                  this.liveData = JSON.parse(event.data);
                  this.dataLoading = false;
                  this.heartCheck(); // Reset heartbeat detection
               } else {
                  // Sometimes it does not recover automatically so have to force a reconnect
                  this.closeSocket();
                  this.heartCheck(10); // Reconnect faster
               }
            };

            this.socket.onopen = function (event) {
                console.log(event);
                console.log("Successfully connected to the echo websocket server...");
            };

            // Listen to window events, when the window closes, take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.closeSocket();
            };
        },

        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                if (this.inverterData) {
                    this.inverterData.forEach(element => {
                        element.data_age++;
                    });
                }
            }, 1000);
        },
        // Send heartbeat packets regularly * 59s Send a heartbeat
        heartCheck(duration: number = 59) {
            this.heartInterval && clearTimeout(this.heartInterval);
            this.heartInterval = setInterval(() => {
                if (this.socket.readyState === 1) {
                    // Connection status
                    this.socket.send("ping");
                } else {
                    this.initSocket(); // Breakpoint reconnection 5 Time
                }
            }, duration * 1000);
        },
        /** To break off websocket Connect */
        closeSocket() {
            if (testmodus == false) { // damit im testmodus kein close-befehl gemacht wird - erzeugt fehler
               this.socket.close();
            }
            this.heartInterval && clearTimeout(this.heartInterval);
            this.isFirstFetchAfterConnect = true;
        },
        onHideEventlog() {
            this.eventLogView.hide();
        },
        onShowEventlog(serial: number) {
            if (testmodus == false) { // damit im nicht-testmodus eine webserver-abfrage gemacht wird
                this.eventLogLoading = true;
                fetch("/api/eventlog/status?inv=" + serial + "&locale=" + this.$i18n.locale, { headers: authHeader() })
                    .then((response) => handleResponse(response, this.$emitter, this.$router))
                    .then((data) => {
                        this.eventLogList = data;
                        this.eventLogLoading = false;
                });
                this.eventLogView.show();
            }
        },
        onHideDevInfo() {
            this.devInfoView.hide();
        },
        onShowDevInfo(serial: number) {
            this.devInfoLoading = true;
            fetch("/api/devinfo/status?inv=" + serial, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.devInfoList = data;
                    this.devInfoList.serial = serial;
                    this.devInfoLoading = false;
                });

            this.devInfoView.show();
        },
        onHideGridProfile() {
            this.devInfoView.hide();
        },
        onShowGridProfile(serial: number) {
            this.gridProfileLoading = true;
            fetch("/api/gridprofile/status?inv=" + serial, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.gridProfileList = data;

                    fetch("/api/gridprofile/rawdata?inv=" + serial, { headers: authHeader() })
                    .then((response) => handleResponse(response, this.$emitter, this.$router))
                    .then((data) => {
                        this.gridProfileRawList = data;
                        this.gridProfileLoading = false;
                    })
                });

            this.gridProfileView.show();
        },

// -------- aktion bei klick auf den button "Display" ------------------------------------------------------------------
// -------- in WebApi_power.cpp wird toggleDisplay aufgerufen, umschaltung bei jedem klick
    async ontoggleDisplay() {
        const displButt = document.getElementById("displaybutton");
        if (displButt) {
            displButt.style.backgroundColor = "orange";  // Sofort orange!
        }
        try {
            const response = await fetch("/toggleDisplay", {
                method: "POST",
                headers: authHeader(),
            });
            if (!response.ok) {
                throw new Error('Toggle display request failed');
            }
        } catch (error) {
            console.error('Toggle display failed:', error);
        } finally { // finally wird IMMER ausgeführt (Erfolg UND Fehler)
            setTimeout(() => location.reload(), 500); // // Reload mit 500 ms Zeit für WebSocket
        }
    },

// ------------ aktion bei klick auf den button "Lora" ------------------------------------------------------------------
// ------------ in WebApi_power.cpp wird toggleLora aufgerufen, umschaltung bei jedem klick
      async ontoggleLora() {
        const lorButt = document.getElementById("lorabutton");
        if (lorButt) {
            lorButt.style.backgroundColor = "orange";  // Sofort orange!
        }
        try {
            const response = await fetch("/toggleLora", {
                method: "POST",
                headers: authHeader(),
            });
            if (!response.ok) {
                throw new Error('Toggle Lora request failed');
            }
        } catch (error) {
            console.error('Toggle display failed:', error);
        } finally { // finally wird IMMER ausgeführt (Erfolg UND Fehler)
            setTimeout(() => location.reload(), 500); // // Reload mit 500 ms Zeit für WebSocket
        }
      },
//--------------------------------------------------------------------------------

        onHideLimitSettings() {
            this.showAlertLimit = false;
        },
        onShowLimitSettings(serial: number) {
            this.targetLimitList.serial = 0;
            this.targetLimitList.limit_value = 0;
            this.targetLimitList.autolimit_value = 100;
            this.targetLimitType = 1;
            this.targetLimitTypeText = this.$t('home.Relative');

            if (testmodus) {
                // Testdaten direkt setzen
                this.currentLimitList = {
                    limit_relative: 20,
                    max_power: 800,
                    limit_set_status: 'Ok'
                } as LimitStatus;
                this.limitSettingLoading = false;
            } else {
                this.limitSettingLoading = true;
                fetch("/api/limit/status", { headers: authHeader() })
                    .then((response) => handleResponse(response, this.$emitter, this.$router))
                    .then((data) => {
                        this.currentLimitList = data[serial];
                        this.targetLimitList.serial = serial;
                        this.targetLimitList.autolimit_value = data[serial].maxleistung ?? 100;
                        this.limitSettingLoading = false;
                    });
            }

            this.limitSettingView.show();
        },
        onSubmitLimit(e: Event) {
            e.preventDefault();

            this.targetLimitList.limit_type = (this.targetLimitPersistent ? 256 : 0) + this.targetLimitType
            const formData = new FormData();
            formData.append("data", JSON.stringify(this.targetLimitList));

            console.log(this.targetLimitList);

            fetch("/api/limit/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        if (response.type == "success") {
                            this.limitSettingView.hide();
                        } else {
                            this.alertMessageLimit = this.$t('apiresponse.' + response.code, response.param);
                            this.alertTypeLimit = response.type;
                            this.showAlertLimit = true;
                        }
                    }
                )
        },
        onSetLimitSettings(setPersistent: boolean) {
            this.targetLimitType = 1;
            this.targetLimitTypeText = this.$t('home.Relative');
            this.targetLimitMin = 0;
            this.targetLimitMax = 100;
            this.targetLimitPersistent = setPersistent;
        },
        async onSetmaxleistung() {
            try {
                const response = await fetch("/api/limit/setmaxleistung", {
                    method: "POST",
                    headers: authHeader(),
                    body: JSON.stringify({ maxleistung: this.targetLimitList.autolimit_value }),
                });
                if (!response.ok) {
                    const text = await response.text();
                    throw new Error(`Setmaxleistung failed: ${response.status} - ${text}`);
                }
            } catch (error) {
                console.error("Setting maxleistung failed:", error);
            }
        },

        /*
        ausgeschaltet, typ ist immer 1 = % relativ
                onSelectType(type: number) {
                    if (type == 1) {
                        this.targetLimitTypeText = this.$t('home.Relative');
                        this.targetLimitMin = 0;
                        this.targetLimitMax = 100;
                    } else {
                        this.targetLimitTypeText = this.$t('home.Absolute');
                        this.targetLimitMin = 0;
                        this.targetLimitMax = (this.currentLimitList.max_power > 0 ? this.currentLimitList.max_power : 2250);
                    }
                    this.targetLimitType = type;
                },
        */

        onShowPowerSettings(serial: number) {
            this.powerSettingLoading = true;
            fetch("/api/power/status", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.successCommandPower = data[serial].power_set_status;
                    this.powerSettingSerial = serial;
                    this.powerSettingLoading = false;
                });
            this.powerSettingView.show();
        },

        onHidePowerSettings() {
            this.powerSettingSerial = 0;
            this.showAlertPower = false;
        },

        onSetPowerSettings(turnOn: boolean, restart = false) {
            let data = {};
            if (restart) {
                data = {
                    serial: this.powerSettingSerial,
                    restart: true,
                };
            } else {
                data = {
                    serial: this.powerSettingSerial,
                    power: turnOn,
                };
            }

            const formData = new FormData();
            formData.append("data", JSON.stringify(data));

            console.log(data);

            fetch("/api/power/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        if (response.type == "success") {
                            this.powerSettingView.hide();
                        } else {
                            this.alertMessagePower = this.$t('apiresponse.' + response.code, response.param);
                            this.alertTypePower = response.type;
                            this.showAlertPower = true;
                        }
                    }
                )
        },
        calculateAbsoluteTime(lastTime: number): string {
            const date = new Date(Date.now() - lastTime * 1000);
            return this.$d(date, 'datetime');
        }
    },
});
</script>


<style>
.btn-group {
    border-radius: var(--bs-border-radius);
    margin-top: 0.25rem;
}
</style>