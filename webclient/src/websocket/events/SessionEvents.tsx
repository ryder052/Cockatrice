import { Room, StatusEnum, User } from "types";

import { SessionCommands } from "../commands";
import { RoomPersistence, SessionPersistence } from '../persistence';
import { ProtobufEvents } from '../services/ProtobufService';
import webClient from '../WebClient';

export const SessionEvents: ProtobufEvents = {
  ".Event_AddToList.ext": addToList,
  ".Event_ConnectionClosed.ext": connectionClosed,
  ".Event_ListRooms.ext": listRooms,
  ".Event_NotifyUser.ext": notifyUser,
  ".Event_PlayerPropertiesChanges.ext": playerPropertiesChanges,
  ".Event_RemoveFromList.ext": removeFromList,
  ".Event_ServerIdentification.ext": serverIdentification,
  ".Event_ServerMessage.ext": serverMessage,
  ".Event_ServerShutdown.ext": serverShutdown,
  ".Event_UserJoined.ext": userJoined,
  ".Event_UserLeft.ext": userLeft,
  ".Event_UserMessage.ext": userMessage,
}

function addToList({ listName, userInfo}: AddToListData) {
  switch (listName) {
    case 'buddy': {
      SessionPersistence.addToBuddyList(userInfo);
      break;
    }
    case 'ignore': {
      SessionPersistence.addToIgnoreList(userInfo);
      break;
    }
    default: {
      console.log('Attempted to add to unknown list: ', listName);
    }
  }
}

function connectionClosed({ reason, reasonStr }: ConnectionClosedData) {
  let message = "";

  // @TODO (5)
  if (reasonStr) {
    message = reasonStr;
  } else {
    switch(reason) {
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.USER_LIMIT_REACHED:
        message = "The server has reached its maximum user capacity";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.TOO_MANY_CONNECTIONS:
        message = "There are too many concurrent connections from your address";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.BANNED:
        message = "You are banned";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.DEMOTED:
        message = "You were demoted";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.SERVER_SHUTDOWN:
        message = "Scheduled server shutdown";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.USERNAMEINVALID:
        message = "Invalid username";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.LOGGEDINELSEWERE:
        message = "You have been logged out due to logging in at another location";
        break;
      case webClient.protobuf.controller.Event_ConnectionClosed.CloseReason.OTHER:
      default:
        message = "Unknown reason";
        break;
    }
  }

  webClient.socket.updateStatus(StatusEnum.DISCONNECTED, message);
}

function listRooms({ roomList }: ListRoomsData) {
  RoomPersistence.updateRooms(roomList);

  if (webClient.options.autojoinrooms) {
    roomList.forEach(({ autoJoin, roomId }) => {
      if (autoJoin) {
        SessionCommands.joinRoom(roomId);
      }
    });
  }
}

function notifyUser(payload) {
  // console.info("Event_NotifyUser", payload);
}

function playerPropertiesChanges(payload) {
  // console.info("Event_PlayerPropertiesChanges", payload);
}

function removeFromList({ listName, userName }: RemoveFromListData) {
  switch (listName) {
    case 'buddy': {
      SessionPersistence.removeFromBuddyList(userName);
      break;
    }
    case 'ignore': {
      SessionPersistence.removeFromIgnoreList(userName);
      break;
    }
    default: {
      console.log('Attempted to remove from unknown list: ', listName);
    }
  }
}

function serverIdentification(info: ServerIdentificationData) {
  const { serverName, serverVersion, protocolVersion } = info;

  if (protocolVersion !== webClient.protocolVersion) {
    SessionCommands.disconnect();
    webClient.socket.updateStatus(StatusEnum.DISCONNECTED, "Protocol version mismatch: " + protocolVersion);
    return;
  }

  webClient.resetConnectionvars();
  webClient.socket.updateStatus(StatusEnum.LOGGINGIN, "Logging in...");
  SessionPersistence.updateInfo(serverName, serverVersion);
  SessionCommands.login();
}

function serverMessage({ message }: ServerMessageData) {
  SessionPersistence.serverMessage(message);
}

function serverShutdown(payload) {
  // console.info("Event_ServerShutdown", payload);
}

function userJoined({ userInfo }: UserJoinedData) {
  SessionPersistence.userJoined(userInfo);
}

function userLeft({ name }: UserLeftData) {
  SessionPersistence.userLeft(name);
}

function userMessage(payload) {
  // console.info("Event_UserMessage", payload);
}

interface SessionEvent {
  sessionEvent: {}
}

interface AddToListData {
  listName: string;
  userInfo: User;
}

interface ConnectionClosedData {
  endTime: number;
  reason: number;
  reasonStr: string;
}

interface ListRoomsData {
  roomList: Room[];
}

interface RemoveFromListData {
  listName: string;
  userName: string;
}

interface ServerIdentificationData {
  protocolVersion: number;
  serverName: string;
  serverVersion: string;
}

interface ServerMessageData {
  message: string;
}

interface UserJoinedData {
  userInfo: User;
}

interface UserLeftData {
  name: string;
}