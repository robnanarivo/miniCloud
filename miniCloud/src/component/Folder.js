import React from "react";
import MenuBar from "./MenuBar";
import File from "./File";
import { useState, useContext, useEffect } from "react";
import { Card, Col, Row, Modal, Input } from "antd";
import {
    getFolderContent,
    createFolder,
    deleteFolder,
    moveFolder,
    uploadFile,
} from "../fetcher";
import {
    DeleteFilled,
    PlusCircleFilled,
    CaretDownFilled,
    CaretRightFilled,
    FileMarkdownFilled,
    CloudUploadOutlined,
} from "@ant-design/icons";

const { Meta } = Card;

const Folder = (props) => {
    const { path, name, refresh } = props;
    const [files, setFiles] = useState([]);
    const [modalStatus, setModalStatus] = useState("");
    const [modalInput, setModalInput] = useState("");
    const [showChildren, setShowChildren] = useState(false);
    const [selectedFile, setSelectedFile] = useState();
    const [isFilePicked, setIsFilePicked] = useState(false);
    useEffect(() => {
        getFolderContent("/" + path.join("/")).then((res) => {
            setFiles(res);

        });
    }, []);
    const handleOk = () => {
        // TODO case modalStatus and change operation

        if (modalStatus == "move folder") {
            moveFolder(path.join("/") + "/", modalInput).then((res) => {
                alert("moved, please refresh page!");
                refreshPage();
            });
        } else if (modalStatus == "create folder") {
            createFolder(path.join("/") + "/", modalInput).then((res) => {
                console.log('path', path)
                alert("created, please refresh page!");
                refreshPage();
            });
        } else if (modalStatus == "upload file" && isFilePicked) {       
            uploadFile('/'+path.join("/") + "/", selectedFile).then((res) => {
                alert("uploaded, please refresh page!");
                refreshPage();
            });
        }

        setModalStatus("");
    };
    const handleCancel = () => {
        setModalStatus("");
    };
    const onModalInput = (e) => {
        setModalInput(e.target.value);
    };
    const changeUpload = (e) => {
        setSelectedFile(e.target.files[0]);
        setIsFilePicked(true);
        // console.log("file: ", e.target.files[0])

    };
    const refreshPage = () => {
        refresh();
    }


    let fileListing = <span>Loading...</span>;
    if (files != null) {
        fileListing = (
            <ul>
                {files.sort().map((file) => {
                    const filepath = [...path, file.name];
                    if (file.isFolder) {
                        return (
                            <Folder
                                key={filepath.join("/")}
                                path={filepath}
                                name={file.name}
                                refresh={()=>refreshPage()}
                            />
                        );
                    } else {
                        return (
                            <File key={filepath.join("/")} path={filepath} name={file.name} refresh={()=>refreshPage()} />
                        );
                    }
                })}
            </ul>
        );

        if (!files.length) {
            fileListing = (
                <ul>
                    <li>
                        <em>Empty folder</em>
                    </li>
                </ul>
            );
        }
    }
    return (
        <li>
            <div style={{ margin: "-18px 0px 0px 5px" }}>
                {name && name.length > 0 ? name : " "}
            </div>
            {modalStatus != "" && (
                <Modal
                    title={modalStatus}
                    open={modalStatus != ""}
                    onOk={() => {
                        handleOk();
                    }}
                    onCancel={handleCancel}
                >
                    {modalStatus != "upload file" && (
                        <Input
                            allowClear
                            onChange={onModalInput}
                            defaultValue={modalInput}
                        />
                    )}
                    {modalStatus == "upload file" &&
                        <div>
                            <input type="file" name="file" onChange={changeUpload} />
                        </div>
                    }
                </Modal>
            )}

            <div className="storage-button-group">
                <div
                    className="move-button"
                    onClick={() => {
                        setModalInput(path.join("/") + "/");
                        setModalStatus("move folder");
                    }}
                >
                    <FileMarkdownFilled />
                </div>
                <div
                    className="delete-button"
                    onClick={() => {
                        deleteFolder(path.join("/")+"/" ).then((res) => {
                            alert("deleted, please refresh page!");
                            refreshPage();
                        });
                    }}
                >
                    <DeleteFilled />
                </div>
                <div
                    className="upload-button"
                    onClick={() => {
                        setModalStatus("upload file");
                    }}
                >
                    <CloudUploadOutlined />
                </div>
                <div
                    className="add-button"
                    onClick={() => {
                        setModalInput("newFolerName");
                        setModalStatus("create folder");
                    }}
                >
                    <PlusCircleFilled />
                </div>
                <div
                    className="expand-button"
                    onClick={() => {
                        setShowChildren(!showChildren);
                    }}
                >
                    {showChildren ? <CaretDownFilled /> : <CaretRightFilled />}
                </div>
            </div>

            {showChildren && fileListing}
        </li>
    );
};

export default Folder;
