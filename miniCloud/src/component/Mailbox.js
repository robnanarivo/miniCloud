import React from "react";
import MenuBar from "./MenuBar";
import { Modal, Input } from "antd";
import { useState, useContext, useEffect } from "react";
import { getAllEmails, sendEmail, deleteEmail } from "../fetcher";
import { } from "../fetcher";
import {
    DeleteFilled,
    SwapRightOutlined,
    SwapLeftOutlined
} from "@ant-design/icons";
const { TextArea } = Input;


var months = [
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
];

class MainMailbox extends React.Component {
    constructor(args) {
        super(args);

        // Assign unique IDs to the emails
        const emails = this.props.allEmails;
        // let id = 0;
        // for (const email of emails) {
        //   email.id = id++;
        // }

        this.state = {
            selectedEmailId: 0,
            currentSection: "inbox",
            emails,
        };
    }

    openEmail(id) {
        const emails = this.state.emails;
        const index = emails.findIndex((x) => x.id === id);
        // emails[index].read = 'true';
        this.setState({
            selectedEmailId: id,
            emails,
        });
    }

    deleteMessage(id) {
        // Mark the message as 'deleted'
        const emails = this.state.emails;
        const index = emails.findIndex((x) => x.id === id);
        emails[index].tag = "deleted";

        // Select the next message in the list
        let selectedEmailId = "";
        for (const email of emails) {
            if (email.tag === this.state.currentSection) {
                selectedEmailId = email.id;
                break;
            }
        }

        this.setState({
            emails,
            selectedEmailId,
        });
        deleteEmail(id).then((res) => {
            alert("email deleted")
        })
    }

    setSidebarSection(section) {
        let selectedEmailId = this.state.selectedEmailId;
        if (section !== this.state.currentSection) {
            selectedEmailId = "";
        }

        this.setState({
            currentSection: section,
            selectedEmailId,
        });
    }

    render() {
        const currentEmail = this.state.emails.find(
            (x) => x.id === this.state.selectedEmailId
        );
        return /*#__PURE__*/ React.createElement(
            "div",
            null /*#__PURE__*/,
            React.createElement(Sidebar, {
                emails: this.state.emails,
                setSidebarSection: (section) => {
                    this.setSidebarSection(section);
                },
            }) /*#__PURE__*/,
            React.createElement(
                "div",
                { className: "inbox-container" } /*#__PURE__*/,
                React.createElement(EmailList, {
                    emails: this.state.emails.filter(
                        (x) => x.tag === this.state.currentSection
                    ),
                    onEmailSelected: (id) => {
                        this.openEmail(id);
                    },
                    selectedEmailId: this.state.selectedEmailId,
                    currentSection: this.state.currentSection,
                    popComposeEmail: (a, b, c) => {
                        this.props.popComposeEmail(a, b, c);
                    },
                }) /*#__PURE__*/,
                React.createElement(EmailDetails, {
                    email: currentEmail,
                    onDelete: (id) => {
                        this.deleteMessage(id);
                    },
                    popComposeEmail: (a, b, c) => {
                        this.props.popComposeEmail(a, b, c);
                    }
                })
            )
        );
    }
}
/* Sidebar */
const Sidebar = ({ emails, setSidebarSection }) => {
    var unreadCount = emails.reduce(
        function (previous, msg) {
            if (msg.read !== "true") {
                return previous + 1;
            } else {
                return previous;
            }
        }.bind(this),
        0
    );

    var deletedCount = emails.reduce(
        function (previous, msg) {
            if (msg.tag === "deleted") {
                return previous + 1;
            } else {
                return previous;
            }
        }.bind(this),
        0
    );

    return /*#__PURE__*/ React.createElement(
        "div",
        { id: "sidebar" } /*#__PURE__*/,
        // React.createElement(
        //   "div",
        //   { className: "sidebar__compose" } /*#__PURE__*/,
        //   React.createElement(
        //     "a",
        //     { href: "#", className: "btn compose" },
        //     "Compose " /*#__PURE__*/,
        //     React.createElement("span", { className: "fa fa-pencil" })
        //   )
        // ) /*#__PURE__*/,
        React.createElement(
            "ul",
            { className: "sidebar__inboxes" } /*#__PURE__*/,
            React.createElement(
                "li",
                {
                    onClick: () => {
                        setSidebarSection("inbox");
                    },
                },
        /*#__PURE__*/ React.createElement(
                    "a",
                    null /*#__PURE__*/,
                    React.createElement("span", { className: "fa fa-inbox" }),
                    " Inbox" /*#__PURE__*/
                    // React.createElement("span", { className: "item-count" }, unreadCount)
                )
            ) /*#__PURE__*/,
        //     React.createElement(
        //         "li",
        //         {
        //             onClick: () => {
        //                 setSidebarSection("sent");
        //             },
        //         },
        // /*#__PURE__*/ React.createElement(
        //             "a",
        //             null /*#__PURE__*/,
        //             React.createElement("span", { className: "fa fa-paper-plane" }),
        //             " Sent" /*#__PURE__*/
        //             // React.createElement("span", { className: "item-count" }, "0")
        //         )
        //     ) /*#__PURE__*/,
        //     React.createElement(
        //         "li",
        //         {
        //             onClick: () => {
        //                 setSidebarSection("drafts");
        //             },
        //         },
        // /*#__PURE__*/ React.createElement(
        //             "a",
        //             null /*#__PURE__*/,
        //             React.createElement("span", { className: "fa fa-pencil-square-o" }),
        //             " Drafts" /*#__PURE__*/
        //             // React.createElement("span", { className: "item-count" }, "0")
        //         )
        //     ) /*#__PURE__*/,

        //     React.createElement(
        //         "li",
        //         {
        //             onClick: () => {
        //                 setSidebarSection("deleted");
        //             },
        //         },
        // /*#__PURE__*/ React.createElement(
        //             "a",
        //             null /*#__PURE__*/,
        //             React.createElement("span", { className: "fa fa-trash-o" }),
        //             " Trash" /*#__PURE__*/
        //             // React.createElement("span", { className: "item-count" }, deletedCount)
        //         )
        //     )
        )
    );
};

/* Email classes */
const EmailListItem = ({ email, onEmailClicked, selected, popComposeEmail }) => {
    let classes = "singleEmail";
    if (selected) {
        classes += " selected";
    }

    return /*#__PURE__*/ React.createElement(
        "div",
        {
            onClick: () => {
                onEmailClicked(email.id);
            },
            className: classes,
        } /*#__PURE__*/,
        React.createElement(
            "div",
            { className: "singleEmail__subject truncate" },
            email.subject
        ) /*#__PURE__*/,
        React.createElement(
            "div",
            { className: "singleEmail__details" } /*#__PURE__*/,
            React.createElement(
                "span",
                { className: "singleEmail__from truncate" },
                email.address
            ) /*#__PURE__*/,
            React.createElement(
                "span",
                { className: "singleEmail__time truncate" },
                // getPrettyDate(email.time)
                email.time
            )
        )
    );
};

const EmailDetails = ({ email, onDelete, popComposeEmail }) => {
    if (!email) {
        return /*#__PURE__*/ React.createElement("div", {
            className: "contentEmail empty",
        });
    }

    // const date = `${getPrettyDate(email.time)} · ${getPrettyTime(email.time)}`;
    const date = email.time;
    const getDeleteButton = () => {
        if (email.tag !== "deleted") {
            return /*#__PURE__*/ React.createElement("span", {
                onClick: () => {
                    onDelete(email.id);
                    // popComposeEmail("fff","dd","s")
                    //TODO fetch delete
                },
                className: "delete-btn fa fa-trash-o",

            }, <DeleteFilled />);
        }
        return undefined;
    };
    const getReplyButton = () => {
        if (email.tag !== "deleted") {
            return /*#__PURE__*/ React.createElement("span", {
                onClick: () => {
                    // onDelete(email.id);
                    popComposeEmail(email.address, "reply: " + email.subject, "------\n" + email.message)
                },
                className: "reply-btn fa fa-trash-o",

            }, <SwapLeftOutlined />);
        }
        return undefined;
    };
    const getForwardButton = () => {
        if (email.tag !== "deleted") {
            return /*#__PURE__*/ React.createElement("span", {
                onClick: () => {
                    // onDelete(email.id);
                    popComposeEmail("", "fwd: " + email.subject, "------\n" + email.message)
                },
                className: "forward-btn fa fa-trash-o",

            }, <SwapRightOutlined />);
        }
        return undefined;
    };
    return /*#__PURE__*/ React.createElement(
        "div",
        { className: "contentEmail" } /*#__PURE__*/,
        React.createElement(
            "div",
            { className: "contentEmail__header" } /*#__PURE__*/,
            React.createElement(
                "h3",
                { className: "contentEmail__subject" },
                email.subject
            ),
            getReplyButton(),
            getForwardButton(),
            getDeleteButton() /*#__PURE__*/,
            React.createElement(
                "div",
                { className: "contentEmail__time" },
                date
            ) /*#__PURE__*/,
            React.createElement(
                "div",
                { className: "contentEmail__from" },
                email.address
            )
        ) /*#__PURE__*/,

        React.createElement(
            "div",
            { className: "contentEmail__message" },
            email.message
        )
    );
};

/* EmailList contains a list of Email components */
const EmailList = ({ emails, onEmailSelected, selectedEmailId, popComposeEmail }) => {
    if (emails.length === 0) {
        return /*#__PURE__*/ React.createElement(
            "div",
            { className: "email-list empty" },
            "empty!"
        );
    }

    return /*#__PURE__*/ React.createElement(
        "div",
        { className: "email-list" },

        emails.map((email) => {
            return /*#__PURE__*/ React.createElement(EmailListItem, {
                onEmailClicked: (id) => {
                    onEmailSelected(id);
                    // popComposeEmail("aaa", "bbb")
                },
                email: email,
                selected: selectedEmailId === email.id,
                popComposeEmail: (a, b, c) => {
                    popComposeEmail(a, b, c)
                },
            });
        })
    );
};

// Render
// $.ajax({ url: 'https://s3-us-west-2.amazonaws.com/s.cdpn.io/311743/dummy-emails.json',
//   type: 'GET',
//   success: function (result) {
//     React.render( /*#__PURE__*/React.createElement(App, { emails: result }), document.getElementById('inbox'));
//   } });

// Helper methods
const getPrettyDate = (date) => {
    date = date.split(" ")[0];
    const newDate = date.split("-");
    const month = months[0];
    return `${month} ${newDate[2]}, ${newDate[0]}`;
};

// Remove the seconds from the time
const getPrettyTime = (date) => {
    const time = date.split(" ")[1].split(":");
    return `${time[0]}:${time[1]}`;
};

const Mailbox = (props) => {
    const [isModalOpen, setIsModalOpen] = useState(false);
    const [recipientInput, setRecipientInput] = useState("");
    const [subjectInput, setSubjectInput] = useState("");
    const [messageInput, setMessageInput] = useState("");
    const [allEmails, setAllEmails] = useState([]);
    const [updateFlag, setUpdateFlag] = useState(true);

    const showModal = () => {
        setIsModalOpen(true);
    };
    const handleOk = () => {
        setIsModalOpen(false);
        sendEmail(recipientInput, subjectInput, messageInput).then((res) => {
            console.log("res", res);
            setUpdateFlag(!updateFlag);
            if (res.status == 201) {
                alert("email sent");
            } else {
                alert("send email failed");
            }
        })
    };
    const handleCancel = () => {
        setMessageInput("")
        setSubjectInput("")
        setRecipientInput("")
        setIsModalOpen(false);
    };
    const onMessageInput = (e) => {
        setMessageInput(e.target.value)
    };
    const onSubjectInput = (e) => {
        setSubjectInput(e.target.value)
    };
    const onRecptInput = (e) => {
        setRecipientInput(e.target.value)
    };
    useEffect(() => {
        getAllEmails().then((res) => {
            setAllEmails(res);
            // console.log("QQ", res);
        });
    }, []);
    useEffect(() => {
        async function sleep() {
            setAllEmails([]);
            await new Promise(resolve => setTimeout(resolve, 1000));
            getAllEmails().then((res) => {
                
                setAllEmails(res);
                // console.log("QQ", res);
            });
          }
        sleep();        
        
    }, [updateFlag]);
    const popComposeEmail = (address, subject, content) => {

        setMessageInput(content)
        setSubjectInput(subject)
        setRecipientInput(address)
        setIsModalOpen(true);
    };
    return (
        <>
            <div
                className="compose-button"
                onClick={() => {
                    showModal();
                }}
            >
                Write Email
            </div>
            {allEmails && allEmails.length > 0 && <MainMailbox allEmails={allEmails} popComposeEmail={popComposeEmail} />}
            <Modal
                title="Write Email"
                open={isModalOpen}
                onOk={() => {
                    handleOk();
                }}
                onCancel={() => {
                    handleCancel();
                }}
            >
                <div>Send To:</div>
                <Input allowClear onChange={onRecptInput} value={recipientInput} />
                <div>Subject:</div>
                <Input allowClear onChange={onSubjectInput} value={subjectInput} />
                <div>Message:</div>
                <TextArea
                    showCount
                    maxLength={1500}
                    style={{
                        height: 120,
                        resize: "none",
                    }}
                    onChange={onMessageInput}
                    value={messageInput}
                />
            </Modal>
        </>
    );
};

export default Mailbox;
