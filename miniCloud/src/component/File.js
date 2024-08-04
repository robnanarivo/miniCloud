import React from "react";
import MenuBar from "./MenuBar";
import { useState, useContext, useEffect } from "react";
import {
    DeleteFilled,
    CloudDownloadOutlined,
    FileMarkdownFilled,
} from "@ant-design/icons";
import { Card, Col, Row, Modal, Input } from "antd";
import { getFile, moveFile, deleteFile } from "../fetcher";
import domain from '../constants';

const { Meta } = Card;

const File = (props) => {
    const { path, name, refresh } = props;
    const [modalStatus, setModalStatus] = useState("");
    const [modalInput, setModalInput] = useState("");
    const handleCancel = () => {
        setModalStatus("");
    };
    const onModalInput = (e) => {
        setModalInput(e.target.value);
    };
    const refreshPage = () => {
        refresh();
    }
    const handleOk = () => {
        // TODO case modalStatus and change operation

        if (modalStatus == "move file") {
            moveFile(path.join("/"), modalInput).then((res) => {
                alert("moved, please refresh page!");
                refreshPage();
            });
        }
        setModalStatus("");
    };

    return (
        <li>
            {modalStatus != "" && (
                <Modal
                    title={modalStatus}
                    open={modalStatus != ""}
                    onOk={() => {
                        handleOk();
                    }}
                    onCancel={handleCancel}
                >
                    {modalStatus == "move file" && (
                        <Input
                            allowClear
                            onChange={onModalInput}
                            defaultValue={modalInput}
                        />
                    )}
                </Modal>
            )}

            <div className="storage-button-group">
                <div
                    className="move-button"
                    onClick={() => {
                        setModalInput(path.join("/"));
                        setModalStatus("move file");
                    }}
                >
                    <FileMarkdownFilled title="This is my tooltip" />
                </div>
                <div
                    className="delete-button"
                    onClick={() => {
                        deleteFile(path.join("/")).then((res) => {
                            alert("deleted, please refresh page!");
                            refreshPage();
                        });
                    }}
                >
                    <DeleteFilled />
                </div>
                <div
                    className="download-button"
                >
                    <a href={`http://${domain}/api/storage/${path.join("/")}?type=file`} download>
                        <CloudDownloadOutlined />
                    </a>
                </div>
            </div>
            {name}
        </li>
    );
};

export default File;
