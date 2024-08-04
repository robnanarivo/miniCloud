import React from "react";
import { Navbar, NavbarBrand, Nav, NavItem, NavLink } from "shards-react";
import { HomeFilled, UserSwitchOutlined } from '@ant-design/icons';
import { useState } from "react";
import { Modal, Input } from "antd";
import { setNewPassword } from "../fetcher";

import "shards-ui/dist/css/shards.min.css";


const MenuBar = (props) => {
    const { status, back_options, username } = props;
    const [isModalOpen, setIsModalOpen] = useState(false);
    const [inputPwd, setInputPwd] = useState("");

    const onPwdInput = (e) => {
        setInputPwd(e.target.value);
    };
    const showModal = () => {
        setIsModalOpen(true);

    };
    const handleOk = () => {
        setIsModalOpen(false);
        //TODO call fetcher with update email
        setNewPassword(username, inputPwd);

        // console.log("inputPwd", username, inputPwd)


    };
    const handleCancel = () => {
        setIsModalOpen(false);
    };
    return (
        <Navbar type="dark" theme="primary" expand="sm">
            <NavbarBrand onClick={() => back_options()}>CIS 505 Penn Cloud</NavbarBrand>
            {status != "login" &&
                <a href="/">
                    <HomeFilled className="home-button" 
                    onClick={() => { document.cookie = ""; window.location('/'); }} />
                </a>}
            {status != "login" && <UserSwitchOutlined className="changepwd-button" onClick={() => showModal()} />}
            <Modal
                title="Change Password"
                open={isModalOpen}
                onOk={() => {
                    handleOk();
                }}
                onCancel={handleCancel}
            >
                <div>New Password:</div>
                <Input allowClear onChange={onPwdInput} />
            </Modal>
        </Navbar>
    );
};

export default MenuBar;
